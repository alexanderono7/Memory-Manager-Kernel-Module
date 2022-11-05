#!/bin/bash
dmesg --clear
insmod memory_manager.ko pid=100
rmmod memory_manager
dmesg
