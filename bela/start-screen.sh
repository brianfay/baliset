#!/bin/sh

cd /root/baliset

screen -d -m -S baliset
screen -S baliset -p 0 -X title audio_server
screen -S baliset -X screen -t ui_server
screen -S baliset -X screen -t shell
screen -S baliset -p 0 -X stuff './build/baliset'
screen -S baliset -p 1 -X stuff 'cd ui/server'
screen -S baliset -p 1 -X stuff 'node ./app.js'
