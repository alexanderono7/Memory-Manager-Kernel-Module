#!/bin/bash
dmesg --clear
insmod memory_manager.ko pid=1
sleep 50
rmmod memory_manager
dmesg
