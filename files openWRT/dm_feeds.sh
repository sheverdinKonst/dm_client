#!/bin/sh

cp -r /home/sheverdin/Fort_Telecom/device_manager_client/src/ /home/sheverdin/Fort_Telecom/t_fortis/t_fortis_pack/openwrt_tfortis_packages/tf_device_monitor  

cp -r /home/sheverdin/Fort_Telecom/device_manager_client/files/etc/tf_device_monitor_scripts /home/sheverdin/Fort_Telecom/t_fortis/t_fortis_pack/openwrt_tfortis_packages/tf_device_monitor/files/etc  


./scripts/feeds update -a 
./scripts/feeds install -a 
   
