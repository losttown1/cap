fx_version 'cerulean'
game 'gta5'

author 'ZeroElite'
description 'Advanced Illegal Car Boosting System - NoPixel Style'
version '2.0.0'

lua54 'yes'

shared_scripts {
    '@ox_lib/init.lua',
    'shared/config.lua',
    'shared/vehicles.lua',
    'shared/locations.lua',
    'shared/utils.lua'
}

client_scripts {
    'client/main.lua',
    'client/contracts.lua',
    'client/missions.lua',
    'client/gps.lua',
    'client/chop.lua',
    'client/vin.lua',
    'client/drone.lua',
    'client/nui.lua',
    'client/guards.lua',
    'client/party.lua',
    'client/scanner.lua'
}

server_scripts {
    '@oxmysql/lib/MySQL.lua',
    'server/main.lua',
    'server/contracts.lua',
    'server/database.lua',
    'server/rewards.lua',
    'server/state.lua',
    'server/party.lua',
    'server/anticheat.lua',
    'server/webhooks.lua'
}

ui_page 'html/index.html'

files {
    'html/index.html',
    'html/css/*.css',
    'html/js/*.js',
    'html/img/*.png',
    'html/img/*.webp',
    'html/fonts/*.ttf',
    'html/fonts/*.woff2'
}

dependencies {
    'qb-core',
    'ox_lib',
    'oxmysql'
}

provides {
    'zr-carboosting'
}
