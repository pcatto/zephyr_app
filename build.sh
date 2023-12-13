#!/bin/bash

cd ~/zephyrproject
source .venv/bin/activate
#west build -b nucleo_g431rb ~/Documents/SE/zephyr_app -d ~/Documents/SE/zephyr_app/build
west build -p always -b nucleo_g431rb ~/Documents/SE/zephyr_app -d ~/Documents/SE/zephyr_app/build

