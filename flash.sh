#!/bin/bash

cd ~/zephyrproject
source .venv/bin/activate
west flash -d ~/zephyr_workspace/zephyr_app/build/

