#!/bin/bash

cd ~/zephyrproject
source .venv/bin/activate
west build -p always ~/zephyr_workspace/zephyr_app -d ~/zephyr_workspace/zephyr_app/build

