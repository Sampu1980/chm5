# CHM6 Data Plane Microservice Guide #


## Introduction ##

The CHM6 Data Plane Microservice (DP MS) handles configuration, fault and PM related to the DCO. It consists of managers which interface with the Redis Server in the northbound direction (through the Application Service Adapter) and adapters that interface with the DCO (through the GRPC Adapter) and the BCM Gearbox in the southbound direction.  

## Managers ##
The managers interface with the Redis Server in the northbound direction (through the Application Service Adapter) and the DCO and GearBox adapters in the southbound direction. The object managers create and initialize the Dco Adapters. They also handle config, fault and pm for each object.

#### DataPlaneMgr ####
- Handles creation and initialization of object managers.
    - Maintains vector of object managers
- Handles initialization of Application Service Adapter (i.e. Redis)
- Handles callbacks from Redis currently, although it is possible this could move to object managers.

#### DpObjectMgr ####
- Base class for object managers

#### DpCardMgr ####
- Handles updates of HAL board proto
- Sends config to DcoCardAdapter

#### DpCarrierMgr ####
- Handles updates of HAL carrier proto
- Sends config to DcoCarrierAdapter
- CLI Menu: dp_mgr -> carrier_mgr

#### DpGigeClientMgr ####
- Handles updates of HAL gige client proto
- Sends config to DcoGigeClientAdapter
- CLI Menu: dp_mgr -> gige_mgr

#### DpOtuMgr ####
- Handles updates of HAL otu proto
- Sends config to DcoOtuAdapter
- CLI Menu: dp_mgr -> otu_mgr

#### DpOduMgr ####
- Handles updates of HAL odu proto
- Sends config to DcoOduAdapter
- CLI Menu: dp_mgr -> odu_mgr

#### DpXcMgr ####
- Handles updates of HAL xcon proto
- Sends config to DcoXconAdapter
- CLI Menu: dp_mgr -> xcon_mgr

## DataPlane Manager Simulator ##
Simulate DataPlane object state and fault changes of DCO
Command Documetation: https://infinera.sharepoint.com/:w:/r/teams/CHM6Software/_layouts/15/Doc.aspx?sourcedoc=%7B4FF93674-DFBD-415C-9B06-FE6E60F4F52A%7D&file=DataplaneMgrSimCmds.docx&action=edit&mobileredirect=true

## Adapters ##
The adapters provide the access to the drivers and FW. The DCO adapters provide access to the DCO Gnxi server. The GearBoxAdapter provides access to the BCM Gearbox Driver.

### DCO Adapters ###

#### DCO Card Adapter ####

#### Dco Carrier Adapter ####

#### DCO GigeClient Adapter ####

#### DCO Odu Adapter ####

#### DCO Otu Adapter ####

#### DCO Xcon Adapter ####

### GearBox Adapter ###

## Environment Variables ##

- chm6_boardinfo - sim, eval or hw
- DcoOecIp     - IP address of dco zynq
- DcoSecProcIp - IP address of dco nxp
- SlotNo       - Slot number of card
- ChassisId    - ID of the chassis

## Build ##

### Versions ###
    - 0.7.37
        ARG chm6devenv=V2.17
        ARG chm6testenv=V1.7.0
        ARG chm6runenv=V2.8
        ARG cli3pp=V0.1
        ARG chm6proto=V4.1.5
        ARG gnmilib=V1.2
        ARG gnmicpp=V1.2
        ARG grpcadapter=V0.11.8
        ARG jsoncppchm6=V1.8.4-infnV1.0
        ARG chm6common=V1.1.8
        ARG chm6_ds_apps_adapter=V1.0.2
        ARG chm6_dco_yang=V0.5.1
        ARG epdm=v2.0.5
        ARG milb=V4.0
        ARG chm6logging=V1.0.3
        ARG chm6serdes=V0.1.1
    - 0.7.34
        ARG chm6devenv=V2.17
        ARG chm6testenv=V1.7.0
        ARG chm6runenv=V2.8
        ARG cli3pp=V0.1
        ARG chm6proto=V4.1.5
        ARG gnmilib=V1.2
        ARG gnmicpp=V1.2
        ARG grpcadapter=V0.11.8
        ARG jsoncppchm6=V1.8.4-infnV1.0
        ARG chm6common=V1.1.8
        ARG chm6_ds_apps_adapter=V1.0.1
        ARG chm6_dco_yang=V0.5.1
        ARG epdm=v2.0.5
        ARG milb=V4.0
        ARG chm6logging=V1.0.3
        ARG chm6serdes=V0.1.1
    - 0.7.15
    - 0.7.14
    - 0.7.13
        ARG chm6devenv=V2.17
        ARG chm6testenv=V1.7.0
        ARG chm6runenv=V2.8
        ARG cli3pp=V0.1
        ARG chm6proto=V4.0.17
        ARG gnmilib=V1.2
        ARG gnmicpp=V1.2
        ARG grpcadapter=V0.11.0
        ARG jsoncppchm6=V1.8.4-infnV1.0
        ARG chm6common=V1.1.8
        ARG chm6_ds_apps_adapter=V0.10.3
        ARG chm6_dco_yang=V0.3.1
        ARG epdm=v2.0.5
        ARG milb=V4.0
        ARG chm6logging=V1.0.3
        
    - 0.7.4
    - 0.7.3
        ARG chm6devenv=V2.17
        ARG chm6testenv=V1.7.0
        ARG chm6runenv=V2.7
        ARG cli3pp=V0.1
        ARG chm6proto=V4.0.9
        ARG gnmilib=V1.2
        ARG gnmicpp=V1.2
        ARG grpcadapter=V0.11.0
        ARG jsoncppchm6=V1.8.4-infnV1.0
        ARG chm6common=V1.1.6
        ARG chm6_ds_apps_adapter=V0.10.0
        ARG chm6_dco_yang=V0.3.0
        ARG epdm=v2.0.5
        ARG milb=V4.0
        ARG chm6logging=V0.1.0

    - 0.7.2
    - 0.7.1
        ARG chm6devenv=V2.17
        ARG chm6testenv=V1.7.0
        ARG chm6runenv=V2.7
        ARG cli3pp=V0.1
        ARG chm6proto=V4.0.7
        ARG gnmilib=V1.2
        ARG gnmicpp=V1.2
        ARG grpcadapter=V0.10.2
        ARG jsoncppchm6=V1.8.4-infnV1.0
        ARG chm6common=V1.1.6
        ARG chm6_ds_apps_adapter=V0.9.1
        ARG chm6_dco_yang=V0.3.0
        ARG epdm=v2.0.5
        ARG milb=V4.0
        ARG chm6logging=V0.1.0

    - 0.7.0
        ARG chm6devenv=V2.17
        ARG chm6testenv=V1.7.0
        ARG chm6runenv=V2.7
        ARG cli3pp=V0.1
        ARG chm6proto=V4.0.7
        ARG gnmilib=V1.2
        ARG gnmicpp=V1.2
        ARG grpcadapter=V0.8.4
        ARG jsoncppchm6=V1.8.4-infnV1.0
        ARG chm6common=V1.1.6
        ARG chm6_ds_apps_adapter=V0.9.1
        ARG chm6_dco_yang=V0.3.0
        ARG epdm=v2.0.5
        ARG milb=V4.0
        ARG chm6logging=V0.1.0

    - 0.6.3
        ARG chm6devenv=V2.17
        ARG chm6testenv=V1.7.0
        ARG chm6runenv=V2.7
        ARG cli3pp=V0.1
        ARG chm6proto=V2.0.0
        ARG gnmilib=V1.2
        ARG gnmicpp=V1.2
        ARG grpcadapter=V0.8.4
        ARG jsoncppchm6=V1.8.4-infnV1.0
        ARG chm6common=V1.1.6
        ARG chm6_ds_apps_adapter=V0.9.1
        ARG chm6_dco_yang=V0.2.1
        
### Agent Compatibility ###
    - 0.6.3  Redis:V1.3.1 + gRPCServer:V0.10.3
    - 0.6.6  Redis:V1.3.1 + gRPCServer:V0.10.3
    - 0.7.3  Redis:V1.4.1 + gRPCServer:V0.11.0
      
### Docker Build Command ###

docker build -t <image_name>:<version> /bld_home/ecina/CHM6//dp_ms_2 --target <build_tgt>

### Build Targets ###

- chm6_dp_ms_dev - Development Environment. Source present but not libs. No build performed.
- chm6_dp_ms_test - TBD
- chm6_dp_ms_build_aarch64 - Arm64 build environment. Source and libs present. Build is performed with Cmake
- ** chm6_dataplane_ms_arm64 ** - Arm64 Runtime Environment. No source or libs. Binary is present.
- chm6_dp_ms_build_x86 - x86 build environment. Source and libs present. Build is performed with Cmake.
- ** chm6_dataplane_ms_x86 ** - x86 Runtime Environment. No source or libs. Binary is present.

### Docker Run (standalone) Command ###

docker run -e BoardInfo=<sim, eval or hw> -e DcoZynqIp=<server name> -itd --name <container name> <image name>:<version>

### Docker Exec ###

#### Open container shell (bash or sh)

docker exec -it <docker name or id> <sh or bash>

### Running (Deploying Stack) ###

#### Download Images ####

- Note that version numbers may change as needed. This is a current snapshot of versions that are known to work together with DP MS testing.

docker pull sv-artifactory.infinera.com/chm6/arm64/chm6_redis:V1.1.0
docker pull sv-artifactory.infinera.com/chm6/arm64/chm6_app_infra_ms:V0.6.0

#### Building Eqpt MS ####

- Note that Eqpt MS is currently only used to execute python scripts and may be removed from this procedure later. Also it is not in artifactory so we have to build it ourselves

git clone https://bitbucket.infinera.com/scm/mar/chm6-equipment.git

docker build -t sv-artifactory.infinera.com/chm6/eqpt_ms_arm64:V1.0.0 --target eqpt_arm64_run $env:CHM6_REPOS\chm6-equipment

#### Deploying Stack ####

docker stack deploy --compose-file <composeFileName.yml> --resolve-image never <name of stack, i.e. chm6>

- note if stack is previously running, you will need to first halt it
- Use this command to halt it

docker stack down <name of stack>

#### Compose file (yml file) ####

- Note this is a sample compose file that can be used to run dp ms in the stack. There may be other variations of this file that will work
- Also note that names and version numbers of images and networks can change

chm6Services.yml
#
version: '3.7'

services:

  redis_server:
    image: sv-artifactory.infinera.com/chm6/arm64/chm6_redis:V1.1.0
    init:
      true
    logging:
      driver: "json-file"
      options:
        max-size: "100K"
        max-file: 5
    volumes:
      - redis_data:/opt/infinera/data
    networks:
      chm6-overlay-ms:
        aliases:
          - chm6_redis_server
      chm6-overlay-infra:
        aliases:
          - chm6_redis_server
    deploy:
      resources:
        limits:
          cpus: '0.20'
          memory: 40M
        reservations:
          cpus: '0.15'
          memory: 35M
      restart_policy:
        delay: 5s
        condition: any
        max_attempts: 0
        window: 30s
      mode: replicated
      replicas: 1
      endpoint_mode: dnsrr
      update_config:
        parallelism: 0
        delay: 5s
        order: start-first
    healthcheck:
      test : [ "NONE" ]
      interval: 30s
      timeout: 5s
      retries: 3
      start_period: 20s
    entrypoint: ["redis-server", "/opt/infinera/redis.conf"]

  grpc_server:
    image: sv-artifactory.infinera.com/chm6/arm64/chm6_app_infra_ms:V0.6.0
    init:
      true
    logging:
      driver: "json-file"
      options:
        max-size: "100K"
        max-file: 5
    networks:
      chm6-overlay-infra:
        aliases:
          - chm6_grpc_server
      chm6-overlay-thanosagent:
        aliases:
          - chm6_grpc_server
    deploy:
      resources:
        limits:
          cpus: '0.30'
          memory: 50M
        reservations:
          cpus: '0.25'
          memory: 40M
      mode: replicated
      replicas: 1
      endpoint_mode: dnsrr
      update_config:
        parallelism: 0
        delay: 5s
        order: start-first
      restart_policy:
        delay: 5s
        condition: any
        max_attempts: 3
        window: 30s
    healthcheck:
      test : [ "NONE" ]
      interval: 30s
      timeout: 5s
      retries: 3
      start_period: 20s
    depends_on:
      - redis_server
    entrypoint: ["python3", "-u", "ms_server.py"]


  dataplane_ms:
    image: dp_ms_arm64:V1.1.0
    init:
      true
    networks:
      - chm6-overlay-ms
    environment:
      - BoardInfo=sim
      - DcoOecIp=ecina_gnxi:50051
    deploy:
      resources:
        limits:
          cpus: '0.95'
          memory: 300M
        reservations:
          cpus: '0.60'
          memory: 200M
      mode: replicated
      replicas: 1
      endpoint_mode: dnsrr
      update_config:
        parallelism: 5
        delay: 5s
        order: start-first
      restart_policy:
        delay: 5s
        condition: on-failure
        max_attempts: 3
        window: 30s
    healthcheck:
      test : [ "NONE" ]
      interval: 30s
      timeout: 5s
      retries: 3
      start_period: 20s
    depends_on:
      - redis_server
    #stdin_open: true
    tty: true
    #entrypoint: ["/bin/sh"]

  eqpt_ms:
    image: ecina_art_eqpt_ms_arm64:V1.1.0
    #image: sv-artifactory.infinera.com/eqpt_ms:demo1.1
    init:
      true
    networks:
      - chm6-overlay-thanosagent
    deploy:
      mode: replicated
      replicas: 1
      endpoint_mode: dnsrr
      update_config:
        parallelism: 5
        delay: 5s
        order: start-first
      restart_policy:
        delay: 5s
        condition: on-failure
        max_attempts: 3
        window: 30s
    healthcheck:
      test : [ "NONE" ]
      interval: 30s
      timeout: 5s
      retries: 3
      start_period: 20s
    depends_on:
      - grpc_server
    tty: true

networks:
  chm6-overlay-ms:
    name: overlay-ms
    attachable: true
  chm6-overlay-thanosagent:
    name: overlay-thanosagent
  chm6-overlay-infra:
    name: overlay-infra
    attachable: true

volumes:
  redis_data:
