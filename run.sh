#!/bin/bash
dmesg --clear
insmod memory_manager.ko pid=1
sleep 3
rmmod memory_manager
dmesg
