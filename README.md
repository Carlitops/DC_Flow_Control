# DC_Flow_Control
Implementation of Dual Connectivity and flow control algorithms

# Content Description
You will find the code for the eNB and for the UE in two separare branches, i.e., BS and UE, respectively.
Each branch has the necessary features to run the Data Planes aspects of Dual Connectiviy.
To sucessully use this implementation you need two hosts for the eNBs, two hosts for UEs, a and one host for the EPC.

# Software/Hardware Requirements
OAI supports certain hardware and software configuration, please checkout the following link for more information: https://gitlab.eurecom.fr/oai/openairinterface5g/-/wikis/OpenAirSystemRequirements
However, we have tested our implementation using:
1. Ubuntu 16.04.1 with 4.15.0-52-low-latency kernel
2. Intel Core i7-4770 CPU @ 3.4GHz processor, 16 GB of RAM
3. USRP B205mini
4. Iperf3 v3.9

# Build Instructions for eNB
1. git clone https://github.com/Carlitops/DC_Flow_Control.git
2. cd DC_Flow_Control
3. git checkout BS
4. source oaienv
5. cd cmake_targets
6. ./build_oai -I -w USRP --eNB

# Build Instructions for UE
1. git clone https://github.com/Carlitops/DC_Flow_Control.git
2. cd DC_Flow_Control
2. git checkout UE
3. source oaienv
4. cd cmake_targets
5. ./build_oai -I -w USRP --UE
6. Install iperf3

# Build Instructions for EPC
1. Please follow this link for further details https://mosaic5g.io/resources/mosaic5g-oai-snaps-tutorial.pdf
2. Install iperf3

# MN/SN Configuration Instructions
The configuration file for MN is dc_MeNB and for SN is dc_SeNB. They are located at BS/ci-scripts/conf_files.
Go to the section DUAL_CONNECTIVITY
1. Set the IP Addresses in subsection DC_LOCAL_ENB_ADDRESS and DC_REMOTE_ENB_ADDRESS
2. Set the Flow Control algorithm using DC_FLOW_CONTROL_TYPE. Set the same algorithm in the MN and SN configuration files
3. For CCW, set DC_Tccw    -> current version supports values from 1 to 10 ms
4. For CCW, set DC_Dq_max  -> default value is 20 ms
5. For CCW, set DC_alpha   -> default value is 0.3
7. For CCW and Delay-based, set CCr -> default value is 5 ms
Set the BH latency at the MN host, if required
8. tc qdisc del dev YOUR_INTERFACE_NAME root netem
9. tc qdisc add dev eth1 root netem delay X2_DELAY_IN_MS
The rest of the parameters are configured according to your testbed environment, for further details pelase refer to https://gitlab.eurecom.fr/oai/openairinterface5g/-/wikis/OpenAirUsage

# mUE/sUE Configuration Instructions
The configuration file for mUE is dc_mue and for sUE is dc_sue. They are located at UE/ci-scripts/conf_files
Go to the section DUAL_CONNECTIVITY_UE
1. Set the IP Addresses in subsection DC_LOCAL_UE_ADDRESS and DC_REMOTE_UE_ADDRESS
2. If you want to enable the 3GPP reordering mechanism, set DC_REORDERING_ENABLED to "yes". Only valid for mUE
3. If the reordering is enabled, set the reordering timeout using DC_ALGORITHM_VALUE
4. If you want to vary the CQI values reported to the eNBs, set DC_CQI_HACK to "yes"

# Usage Instructions
1. Run the EPC acoording to https://mosaic5g.io/resources/mosaic5g-oai-snaps-tutorial.pdf
In the MN host: 
2. cd cmake_targets/lte_build_oai/build 
3. ./lte-softmodem -O /root/DC_Flow_Control/ci-scripts/conf_files/dc_MeNB.conf
In the SN host: 
4. cd cmake_targets/lte_build_oai/build 
5. ./lte-softmodem -O /root/DC_Flow_Control/ci-scripts/conf_files/dc_SeNB.conf
In the mUE host:
6. cd cmake_targets/ran_build/build 
7. ./lte-uesoftmodem -C 2680000000 -r 50 --ue-rxgain 125 --ue-txgain 0 --ue-scan-carrier --ue-max-power 0 --nokrnmod 1 -O /root/DC_Flow_Control/ci-scripts/conf_files/dc_mue.conf
In the sUE host:
8. cd cmake_targets/ran_build/build 
9. ./lte-uesoftmodem -C 2630000000 -r 50 --ue-rxgain 125 --ue-txgain 0 --ue-scan-carrier --ue-max-power 0 --nokrnmod 1 -O /root/DC_Flow_Control/ci-scripts/conf_files/dc_sue.conf
10. Start the iperf3 server in the mUE host
11. Start the iperf3 client in the EPC host

For any question, please send an email to carlos.pupiales@upc.edu
