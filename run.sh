#!/bin/bash
dmesg --clear
insmod memory_manager.ko pid=666
rmmod memory_manager
dmesg
