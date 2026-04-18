// ═══════════════════════════════════════════════════════════════════════════════
// BOOSTING TABLET - UI LOGIC
// ═══════════════════════════════════════════════════════════════════════════════

const App = {
    isOpen: false,
    playerData: null,
    contracts: [],
    store: [],
    activeContract: null,
    selectedClass: null,
    selectedType: null,
    party: null
};

// ═══════════════════════════════════════════════════════════════════════════════
// NUI MESSAGE HANDLER
// ═══════════════════════════════════════════════════════════════════════════════

window.addEventListener('message', (event) => {
    const data = event.data;
    
    switch (data.action) {
        case 'open':
            openTablet(data.data);
            break;
        case 'close':
            closeTablet();
            break;
        case 'updatePlayerData':
            updatePlayerData(data.data);
            break;
        case 'updateTimer':
            updateTimer(data.data);
            break;
        case 'contractStarted':
            contractStarted(data.data);
            break;
        case 'contractCompleted':
            contractCompleted(data.data);
            break;
        case 'contractFailed':
            contractFailed(data.data);
            break;
        case 'gpsDisabled':
            updateGPSStatus(true);
            break;
        case 'partyUpdate':
            updateParty(data.data);
            break;
        case 'levelUp':
            showLevelUp(data.data.level);
            break;
    }
});

// ═══════════════════════════════════════════════════════════════════════════════
// TABLET OPEN/CLOSE
// ═══════════════════════════════════════════════════════════════════════════════

function openTablet(data) {
    App.isOpen = true;
    App.playerData = data.player;
    App.contracts = data.contracts || [];
    App.store = data.store || [];
    App.activeContract = data.activeContract;
    App.party = data.party;
    
    document.getElementById('app').classList.remove('hidden');
    
    updatePlayerData(data.player);
    renderContracts();
    renderStore();
    updateParty(data.party);
    
    if (App.activeContract) {
        showActiveContract();
    }
}

function closeTablet() {
    App.isOpen = false;
    document.getElementById('app').classList.add('hidden');
    fetch(`https://${GetParentResourceName()}/close`, {
        method: 'POST',
        body: JSON.stringify({})
    });
}

// ═══════════════════════════════════════════════════════════════════════════════
// UPDATE PLAYER DATA
// ═══════════════════════════════════════════════════════════════════════════════

function updatePlayerData(data) {
    if (!data) return;
    
    App.playerData = data;
    
    document.getElementById('playerLevel').textContent = data.level;
    document.getElementById('playerPoints').textContent = formatNumber(data.points);
    document.getElementById('playerCurrency').textContent = '$' + formatNumber(data.currency);
    document.getElementById('heatValue').textContent = data.heat + '%';
    document.getElementById('levelProgress').style.width = data.progressPercent + '%';
    
    // Update heat indicator color
    const heatIndicator = document.getElementById('heatIndicator');
    if (data.heat >= 75) {
        heatIndicator.style.background = 'rgba(255, 62, 62, 0.3)';
    } else if (data.heat >= 50) {
        heatIndicator.style.background = 'rgba(255, 170, 0, 0.3)';
    } else {
        heatIndicator.style.background = '';
    }
    
    // Update stats
    document.getElementById('completedContracts').textContent = data.completedContracts || 0;
    document.getElementById('failedContracts').textContent = data.failedContracts || 0;
    
    const total = (data.completedContracts || 0) + (data.failedContracts || 0);
    const rate = total > 0 ? Math.round((data.completedContracts / total) * 100) : 0;
    document.getElementById('successRate').textContent = rate + '%';
}

// ═══════════════════════════════════════════════════════════════════════════════
// RENDER CONTRACTS
// ═══════════════════════════════════════════════════════════════════════════════

function renderContracts() {
    const grid = document.getElementById('contractsGrid');
    grid.innerHTML = '';
    
    // Sort by class order
    const classOrder = ['D', 'C', 'B', 'A', 'S', 'S+'];
    const sortedContracts = App.contracts.sort((a, b) => {
        return classOrder.indexOf(a.class) - classOrder.indexOf(b.class);
    });
    
    sortedContracts.forEach(contract => {
        const card = document.createElement('div');
        card.className = `contract-card class-${contract.class.toLowerCase().replace('+', 'plus')}`;
        card.style.setProperty('--class-color', contract.color);
        
        if (contract.locked) {
            card.classList.add('locked');
        }
        
        const rewardText = `$${formatNumber(contract.rewards.money?.min || 0)} - $${formatNumber(contract.rewards.money?.max || 0)}`;
        const policeClass = contract.enoughPolice ? '' : 'insufficient';
        
        card.innerHTML = `
            <div class="contract-class">${contract.class}</div>
            <div class="contract-label">${contract.label}</div>
            <div class="contract-rewards">${rewardText}</div>
            <div class="contract-police ${policeClass}">
                <i class="fas fa-shield-alt"></i>
                ${contract.currentPolice}/${contract.requiredPolice} Police
            </div>
            ${contract.locked ? '<i class="fas fa-lock lock-icon"></i>' : ''}
        `;
        
        if (!contract.locked) {
            card.addEventListener('click', () => openContractModal(contract));
        } else {
            card.title = `Requires: Level ${contract.lockRequirement}`;
        }
        
        grid.appendChild(card);
    });
}

// ═══════════════════════════════════════════════════════════════════════════════
// CONTRACT MODAL
// ═══════════════════════════════════════════════════════════════════════════════

function openContractModal(contract) {
    App.selectedClass = contract;
    App.selectedType = contract.contractTypes[0]?.id;
    
    const modal = document.getElementById('contractModal');
    modal.classList.remove('hidden');
    
    document.getElementById('modalTitle').textContent = contract.label + ' Contract';
    
    // Vehicle preview
    if (contract.previewVehicle) {
        const imgPath = `img/${contract.previewVehicle.image}.webp`;
        document.getElementById('vehicleImage').src = imgPath;
        document.getElementById('vehicleImage').onerror = function() {
            this.src = `img/${contract.previewVehicle.image}.png`;
            this.onerror = function() {
                this.src = 'img/default.png';
            };
        };
        document.getElementById('vehicleName').textContent = contract.previewVehicle.label;
        document.getElementById('vehicleBrand').textContent = contract.previewVehicle.brand;
    }
    
    // Contract types
    const typesContainer = document.getElementById('contractTypes');
    typesContainer.innerHTML = '';
    
    contract.contractTypes.forEach((type, index) => {
        const btn = document.createElement('button');
        btn.className = 'type-btn' + (index === 0 ? ' active' : '');
        btn.innerHTML = `<i class="fas ${type.icon}"></i> ${type.label}`;
        
        if (!type.available) {
            btn.disabled = true;
            btn.title = `Requires Level ${type.requiredLevel}`;
        } else {
            btn.addEventListener('click', () => selectContractType(type.id, btn));
        }
        
        typesContainer.appendChild(btn);
    });
    
    // Rewards
    const rewardsGrid = document.getElementById('rewardsGrid');
    rewardsGrid.innerHTML = `
        <div class="reward-item">
            <i class="fas fa-dollar-sign"></i>
            $${formatNumber(contract.rewards.money?.min)} - $${formatNumber(contract.rewards.money?.max)}
        </div>
        <div class="reward-item">
            <i class="fas fa-star"></i>
            ${contract.rewards.points?.min} - ${contract.rewards.points?.max} XP
        </div>
        <div class="reward-item">
            <i class="fas fa-coins"></i>
            $${formatNumber(contract.rewards.currency?.min)} - $${formatNumber(contract.rewards.currency?.max)}
        </div>
    `;
    
    // Requirements
    const requirements = document.getElementById('requirements');
    requirements.innerHTML = `
        <div class="requirement">
            <i class="fas fa-shield-alt"></i>
            ${contract.requiredPolice} Police
        </div>
        <div class="requirement">
            <i class="fas fa-clock"></i>
            ${formatTime(contract.timeLimit)}
        </div>
    `;
}

function selectContractType(typeId, btn) {
    App.selectedType = typeId;
    
    document.querySelectorAll('.type-btn').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
}

function closeContractModal() {
    document.getElementById('contractModal').classList.add('hidden');
    App.selectedClass = null;
    App.selectedType = null;
}

function acceptContract() {
    if (!App.selectedClass || !App.selectedType) return;
    
    fetch(`https://${GetParentResourceName()}/acceptContract`, {
        method: 'POST',
        body: JSON.stringify({
            class: App.selectedClass.class,
            contractType: App.selectedType
        })
    });
    
    closeContractModal();
}

// ═══════════════════════════════════════════════════════════════════════════════
// RENDER STORE
// ═══════════════════════════════════════════════════════════════════════════════

function renderStore() {
    const grid = document.getElementById('storeGrid');
    grid.innerHTML = '';
    
    App.store.forEach(item => {
        const canAfford = App.playerData?.currency >= item.price;
        const meetsLevel = App.playerData?.level >= item.requiredLevel;
        
        const card = document.createElement('div');
        card.className = 'store-item';
        
        card.innerHTML = `
            <img src="img/${item.image}" alt="${item.label}" onerror="this.src='img/default.png'">
            <h4>${item.label}</h4>
            <p>${item.description}</p>
            <div class="price">
                <i class="fas fa-coins"></i>
                $${formatNumber(item.price)}
            </div>
            <button class="btn btn-primary btn-small" 
                    ${(!canAfford || !meetsLevel) ? 'disabled' : ''}
                    onclick="purchaseItem('${item.id}')">
                ${!meetsLevel ? 'Lvl ' + item.requiredLevel : (canAfford ? 'Purchase' : 'Insufficient Funds')}
            </button>
        `;
        
        grid.appendChild(card);
    });
}

function purchaseItem(itemId) {
    fetch(`https://${GetParentResourceName()}/purchaseItem`, {
        method: 'POST',
        body: JSON.stringify({ itemId })
    });
}

// ═══════════════════════════════════════════════════════════════════════════════
// PARTY/CREW
// ═══════════════════════════════════════════════════════════════════════════════

function updateParty(data) {
    if (!data) return;
    
    App.party = data;
    
    const membersContainer = document.getElementById('crewMembers');
    const inviteSection = document.getElementById('inviteSection');
    const createBtn = document.getElementById('createCrewBtn');
    
    if (data.inParty) {
        createBtn.textContent = 'Leave Crew';
        createBtn.onclick = leaveParty;
        
        membersContainer.innerHTML = '';
        data.members.forEach(member => {
            const div = document.createElement('div');
            div.className = 'crew-member';
            div.innerHTML = `
                <div class="avatar">
                    <i class="fas fa-user"></i>
                </div>
                <div class="name">${member.name}</div>
                <div class="role">${member.isLeader ? 'Leader' : 'Member'}</div>
            `;
            membersContainer.appendChild(div);
        });
        
        if (data.isLeader) {
            inviteSection.classList.remove('hidden');
            loadOnlinePlayers();
        } else {
            inviteSection.classList.add('hidden');
        }
    } else {
        createBtn.innerHTML = '<i class="fas fa-plus"></i> Create Crew';
        createBtn.onclick = createParty;
        
        membersContainer.innerHTML = `
            <div class="no-crew">
                <i class="fas fa-users-slash"></i>
                <p>You are not in a crew</p>
            </div>
        `;
        
        inviteSection.classList.add('hidden');
    }
}

function createParty() {
    fetch(`https://${GetParentResourceName()}/createParty`, {
        method: 'POST',
        body: JSON.stringify({})
    });
}

function leaveParty() {
    fetch(`https://${GetParentResourceName()}/leaveParty`, {
        method: 'POST',
        body: JSON.stringify({})
    });
}

function loadOnlinePlayers() {
    fetch(`https://${GetParentResourceName()}/getOnlinePlayers`, {
        method: 'POST',
        body: JSON.stringify({})
    }).then(r => r.json()).then(players => {
        const list = document.getElementById('playerList');
        list.innerHTML = '';
        
        players.forEach(player => {
            const div = document.createElement('div');
            div.className = 'player-item';
            div.innerHTML = `
                <span>${player.name}</span>
                <button class="btn btn-small btn-primary" onclick="invitePlayer(${player.id})">
                    Invite
                </button>
            `;
            list.appendChild(div);
        });
        
        if (players.length === 0) {
            list.innerHTML = '<p style="color: var(--text-muted)">No players available</p>';
        }
    });
}

function invitePlayer(playerId) {
    fetch(`https://${GetParentResourceName()}/invitePlayer`, {
        method: 'POST',
        body: JSON.stringify({ playerId })
    });
}

// ═══════════════════════════════════════════════════════════════════════════════
// ACTIVE CONTRACT
// ═══════════════════════════════════════════════════════════════════════════════

function showActiveContract() {
    const overlay = document.getElementById('activeContract');
    overlay.classList.remove('hidden');
    
    document.getElementById('activeClass').textContent = App.activeContract.class;
    document.getElementById('activeTitle').textContent = 'Active Contract';
}

function hideActiveContract() {
    document.getElementById('activeContract').classList.add('hidden');
}

function contractStarted(data) {
    App.activeContract = data.contract;
    showActiveContract();
}

function contractCompleted(data) {
    App.activeContract = null;
    hideActiveContract();
}

function contractFailed(data) {
    App.activeContract = null;
    hideActiveContract();
}

function updateTimer(data) {
    document.getElementById('contractTimer').textContent = data.formatted;
    
    // Flash when low time
    const timer = document.getElementById('contractTimer');
    if (data.timeRemaining <= 60) {
        timer.style.color = 'var(--error)';
    } else {
        timer.style.color = '';
    }
}

function updateGPSStatus(disabled) {
    const status = document.getElementById('gpsStatus');
    if (disabled) {
        status.classList.add('disabled');
        status.innerHTML = '<i class="fas fa-check-circle"></i> GPS Disabled';
    } else {
        status.classList.remove('disabled');
        status.innerHTML = '<i class="fas fa-satellite-dish"></i> GPS Active';
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// LEVEL UP EFFECT
// ═══════════════════════════════════════════════════════════════════════════════

function showLevelUp(level) {
    const overlay = document.createElement('div');
    overlay.className = 'level-up-overlay';
    overlay.innerHTML = `
        <div class="level-up-content">
            <i class="fas fa-arrow-up"></i>
            <h2>LEVEL UP!</h2>
            <span class="new-level">${level}</span>
        </div>
    `;
    
    document.body.appendChild(overlay);
    
    setTimeout(() => overlay.remove(), 3000);
}

// ═══════════════════════════════════════════════════════════════════════════════
// UTILITIES
// ═══════════════════════════════════════════════════════════════════════════════

function formatNumber(num) {
    if (!num) return '0';
    return num.toString().replace(/\B(?=(\d{3})+(?!\d))/g, ',');
}

function formatTime(seconds) {
    const mins = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
}

function GetParentResourceName() {
    return window.GetParentResourceName ? window.GetParentResourceName() : 'zr-carboosting';
}

// ═══════════════════════════════════════════════════════════════════════════════
// EVENT LISTENERS
// ═══════════════════════════════════════════════════════════════════════════════

document.addEventListener('DOMContentLoaded', () => {
    // Close button
    document.getElementById('closeBtn').addEventListener('click', closeTablet);
    
    // Modal buttons
    document.getElementById('closeModal').addEventListener('click', closeContractModal);
    document.getElementById('cancelContract').addEventListener('click', closeContractModal);
    document.getElementById('acceptContract').addEventListener('click', acceptContract);
    
    // Navigation
    document.querySelectorAll('.nav-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const tab = btn.dataset.tab;
            
            // Update nav buttons
            document.querySelectorAll('.nav-btn').forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            
            // Update tab content
            document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
            document.getElementById(tab + '-tab').classList.add('active');
            
            // Load players if crew tab
            if (tab === 'crew' && App.party?.isLeader) {
                loadOnlinePlayers();
            }
        });
    });
    
    // ESC key
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') {
            if (!document.getElementById('contractModal').classList.contains('hidden')) {
                closeContractModal();
            } else {
                closeTablet();
            }
        }
    });
});
