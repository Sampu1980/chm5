#
# docker build -t sv-artifactory.infinera.com/chm6/x86_64/chm6_dataplane_ms:[version] --target=chm6_dataplane_ms_x86 .
# docker build -t sv-artifactory.infinera.com/chm6/arm64/chm6_dataplane_ms:[version] --target=chm6_dataplane_ms_arm64 .

ARG chm6devenv=V4.1.0
ARG baseimg=0.3.0
ARG chm6runenv=V4.3.3
ARG cli3pp=V0.1
ARG chm6proto=V6.38.34
ARG dcosecprocproto=V2.2.0
ARG gnmilib=V1.2-infV0.1
ARG gnmicpp=V1.2-infV0.1
ARG grpcadapter=V2.6.2
ARG jsoncppchm6=V1.8.4-infnV1.0
ARG chm6common=V1.28.47
# We need to build two binaries with old and new BCM SDK to support
# NSA  SW upgrade as new BCM SDK required a cold boot upgrade
# See GX-31240 for more details.
# BCM SDK for R4.x
ARG epdm_prev_rel=V4.06A
ARG milb_prev_rel=V5.3A
# New BCM SDK
ARG epdm=V4.1.5
ARG milb=V5.6
ARG ds_apps_adapter=2.8.0-alpha.45
ARG chm6_dco_yang=V3.0.2
ARG chm6logging=V2.0.0
ARG chm6serdes=V0.2.6
ARG signing_action=sign
ARG signingkit_version=3.0
ARG fdrlib_ms=R1.14.401

ARG PROTO_RELEASE_BASE=/home/infinera/

ARG opentracing=1.5.1
ARG lightstep=0.14.0
ARG tracer=2.1.0-alpha.1
ARG artifact_root_dir=/home/infinera/artifacts

#------------------------BUILD SRC---------------------------------
FROM sv-artifactory.infinera.com/3pp/cli:${cli3pp} as cli_3pp
FROM sv-artifactory.infinera.com/chm6_proto:${chm6proto} as chm6_proto
FROM sv-artifactory.infinera.com/dco6_secproc_proto:${dcosecprocproto} as dco6_secproc_proto
FROM sv-artifactory.infinera.com/3pp/jsoncpp_chm6:${jsoncppchm6} as jsoncpp_chm6
FROM sv-artifactory.infinera.com/chm6/arm64/chm6_common:${chm6common} as chm6_common_arm64
FROM sv-artifactory.infinera.com/chm6/x86_64/chm6_common:${chm6common} as chm6_common_x86

FROM sv-artifactory.infinera.com/lccmninf/ds_apps_adapter/aarch64:${ds_apps_adapter} as ds_apps_adapter_aarch64
FROM sv-artifactory.infinera.com/lccmninf/ds_apps_adapter/x86_64:${ds_apps_adapter} as ds_apps_adapter_x86_64
FROM sv-artifactory.infinera.com/chm6/arm64/chm6_grpc_dpms_adapter:${grpcadapter} as grpc_adapter_arm64
FROM sv-artifactory.infinera.com/chm6/x86_64/chm6_grpc_dpms_adapter:${grpcadapter} as grpc_adapter_x86
FROM sv-artifactory.infinera.com/chm6/arm64/chm6_logging:${chm6logging} as chm6_logging_arm64
FROM sv-artifactory.infinera.com/chm6/x86_64/chm6_logging:${chm6logging} as chm6_logging_x86
FROM sv-artifactory.infinera.com/3pp/gnmi:${gnmilib} as gnmi_lib
FROM sv-artifactory.infinera.com/3pp/gnmi:${gnmicpp} as gnmi_cpp
FROM sv-artifactory.infinera.com/chm6/dco-yang:${chm6_dco_yang} as dco_yang
FROM sv-artifactory.infinera.com/3pp/epdm:${epdm_prev_rel} as epdm_prev_rel
FROM sv-artifactory.infinera.com/3pp/epdm:${epdm} as epdm
FROM sv-artifactory.infinera.com/3pp/milb:${milb_prev_rel} as milb_prev_rel
FROM sv-artifactory.infinera.com/3pp/milb:${milb} as milb
FROM sv-artifactory.infinera.com/chm6/arm64/chm6_serdes:${chm6serdes} as chm6_serdes_arm64
FROM sv-artifactory.infinera.com/chm6/x86_64/chm6_serdes:${chm6serdes} as chm6_serdes_x86
FROM sv-artifactory.infinera.com/chm6/arm64/chm6_fdrlib_ms:${fdrlib_ms} as fdrlib_ms_arm64

FROM  sv-artifactory.infinera.com/3pp/opentracing:${opentracing} as opentracing
FROM  sv-artifactory.infinera.com/3pp/lightstep:${lightstep} as lightstep
# TODO support fdrlib for x86
FROM sv-artifactory.infinera.com/chm6/x86_64/chm6_fdrlib_ms:V1.6.0 as fdrlib_ms_x86

FROM sv-artifactory.infinera.com/lccmninf/tracer/x86_64:${tracer} as tracer_x86_64
FROM sv-artifactory.infinera.com/lccmninf/tracer/aarch64:${tracer} as tracer_aarch64

FROM sv-artifactory.infinera.com/devopsinfn/baseimg/x86_64/dev:${baseimg} as baseimg

#------------------------------DEV IMAGE----------------------------------------#
FROM sv-artifactory.infinera.com/infnbase/x86_64/chm6devenv:${chm6devenv} as chm6_dp_ms_dev

ARG artifact_root_dir
ARG arch=aarch64

ARG PROTO_RELEASE_BASE
ARG chm6_proto_path=${PROTO_RELEASE_BASE}/chm6_proto
ARG dco6_secproc_proto_path=${PROTO_RELEASE_BASE}/dco6_secproc_proto
ARG proto_base_path=/home/infinera/proto
ARG proto_gen_path=/home/infinera/src_gen/cpp

WORKDIR /home/infinera

#Copy build environment
COPY ./ /home/infinera/

# Install telnet - temporary
WORKDIR /home/infinera/src
RUN dpkg -i telnet_0.17-41.2_amd64.deb

# Add cli
RUN mkdir -p /home/src/external-repos/cli/include
COPY --from=cli_3pp [ \
  "/home/infinera/cli/include", \
  "/home/infinera/external-repos/cli/include" \
  ]
COPY --from=cli_3pp [ \
  "/home/infinera/cli/CMakeLists.txt", \
  "/home/infinera/cli/cliConfig.cmake.in", \
  "/home/infinera/cli/cli.pc.in", \
  "/home/infinera/external-repos/cli/" \
  ]

# Copy interface proto files and generate language bindings
WORKDIR ${proto_gen_path}
WORKDIR ${proto_base_path}
COPY --from=dco6_secproc_proto [ \
  "${dco6_secproc_proto_path}/infinera", \
  "${proto_base_path}/infinera" \
  ]
COPY --from=chm6_proto [ \
  "${chm6_proto_path}/infinera", \
  "${proto_base_path}/infinera" \
  ]

# hadolint ignore=SC2046
RUN protoc --proto_path=. --cpp_out=${proto_gen_path} $(find . -name '*.proto')

COPY --from=chm6_proto [ \
  "/home/infinera/chm6_proto/gen/cpp/", \
  "/home/infinera/src_gen/cpp" \
  ]

# Copy Generate CPP from DCO Yang files
WORKDIR /home/infinera
# TODO Copy from local disk
COPY --from=dco_yang [ \
    "/home/infinera/src_gen/dco", \
    "/home/infinera/src_gen/dco" \
    ]


# Add Chm6 Logging
RUN mkdir -p /home/infinera/src_art/logging/include
COPY --from=chm6_logging_x86 [ \
  "/home/infinera/include", \
  "/home/infinera/src_art/logging/include" \
  ]



WORKDIR /root/go/src/
# hadolint ignore=SC2046,SC2061,SC2035
RUN protoc -I=. -I=/root/go/src/ --cpp_out=. github.com/openconfig/ygot/proto/ywrapper/ywrapper.proto
# hadolint ignore=SC2046,SC2061,SC2035
RUN protoc -I=. -I=/root/go/src/ --cpp_out=. github.com/openconfig/ygot/proto/yext/yext.proto
WORKDIR /home/infinera


#get gnmi related header file
COPY --from=gnmi_cpp [ \
  "/home/infinera/gnmi_gen/cpp/", \
  "/home/infinera/gnmi_gen/cpp/github.com/openconfig/gnmi/proto/gnmi/gnmi.pb.h", \
  "/home/infinera/gnmi_gen/cpp/github.com/openconfig/gnmi/proto/gnmi_ext/gnmi_ext.pb.h", \
  "/home/infinera/gnmi_gen/grpc/github.com/openconfig/gnmi/proto/gnmi/gnmi.grpc.pb.h", \
  "/home/infinera/grpc_client_adapter/cpp/" \
  ]

COPY --from=grpc_adapter_arm64 [ \
  "/home/infinera/include/", \
  "/home/infinera/grpc_client_adapter/cpp" \
  ]

COPY --from=jsoncpp_chm6 [ \
  "/home/infinera/jsoncpp/include/json", \
  "/usr/local/include/json" \
  ]

#-------------------------Prebuild aarch64 -----------------------------------#
FROM chm6_dp_ms_dev as chm6_dp_ms_prebuild_aarch64

ARG artifact_root_dir
ARG arch=aarch64
# Add common modules
RUN mkdir -p /home/src/include
COPY --from=chm6_common_arm64 [ \
  "/home/src/build/aarch64/include", \
  "/home/src/include" \
  ]

COPY --from=chm6_common_arm64 [ \
  "/home/src/build/aarch64/include/nor_include", \
  "/home/src/include" \
  ]

COPY --from=chm6_common_arm64 [ \
  "/home/src/build/aarch64/include/keymgmt_include", \
  "/home/src/include" \
  ]

COPY --from=chm6_common_arm64 [ \
  "/home/src/build/aarch64/include/gecko_include", \
  "/home/src/include" \
  ]

COPY --from=chm6_common_arm64 [ \
  "/home/src/build/aarch64/lib", \
  "/usr/local/lib" \
  ]

COPY --from=chm6_serdes_arm64 [ \
  "/home/src/build/aarch64/include", \
  "/home/src/include" \
  ]
COPY --from=chm6_serdes_arm64 [ \
  "/home/src/build/aarch64/lib", \
  "/usr/local/lib" \
  ]

COPY --from=ds_apps_adapter_aarch64 [ \
  "/home/infinera/build/aarch64/lib/*", \
  "/usr/local/lib/" \
  ]

COPY --from=ds_apps_adapter_aarch64 [ \
  "/home/infinera/build/aarch64/include", \
  "/usr/local/include/" \
  ]

# Copy GRPC_adapter client lib files
COPY --from=grpc_adapter_arm64 [ \
  "/home/infinera/lib/arm/libchm6grpcadapter.a", \
  "/usr/local/lib/" \
  ]

COPY --from=opentracing [ \
  "${artifact_root_dir}/${arch}-linux-gnu/lib", \
  "/usr/lib/${arch}-linux-gnu/" \
]

COPY --from=opentracing [ \
  "${artifact_root_dir}/${arch}-linux-gnu/include", \
  "/usr/local/include/${arch}-linux-gnu/" \
]

COPY --from=lightstep [ \
  "${artifact_root_dir}/${arch}-linux-gnu/lib", \
  "/usr/lib/${arch}-linux-gnu/" \
]

COPY --from=lightstep [ \
  "${artifact_root_dir}/${arch}-linux-gnu/include", \
  "/usr/local/include/${arch}-linux-gnu/" \
]

COPY --from=tracer_aarch64 [ \
  "${artifact_root_dir}/lib/${arch}-linux-gnu/", \
  "/usr/lib/${arch}-linux-gnu/" \
]

COPY --from=tracer_aarch64 [ \
  "${artifact_root_dir}/include/${arch}-linux-gnu/", \
  "/usr/local/include/${arch}-linux-gnu/" \
]

#get gnmi static lib
COPY --from=gnmi_lib [ \
  "/home/Infinera/gnmi_gen/build/aarch64/lib/libgnmi.a", \
  "/usr/local/lib/" \
  ]

# Get json cpp for c++ desrialization
COPY --from=jsoncpp_chm6 [ \
  "/home/infinera/lib_jsoncpp/arm/libjsoncpp.a", \
  "/usr/local/lib/" \
  ]

COPY --from=epdm_prev_rel [ \
  "/home/infinera/lib", \
  "/usr/local/lib/aarch64-linux-gnu/epdm_prev_rel" \
  ]

COPY --from=epdm [ \
  "/home/infinera/lib", \
  "/usr/local/lib/aarch64-linux-gnu/epdm" \
  ]

COPY --from=milb_prev_rel [ \
  "/home/infinera/lib", \
  "/usr/local/lib/aarch64-linux-gnu/milb_prev_rel" \
  ]

COPY --from=milb [ \
  "/home/infinera/lib", \
  "/usr/local/lib/aarch64-linux-gnu/milb" \
  ]

COPY --from=epdm_prev_rel [ \
  "/home/infinera/include/", \
  "/usr/local/include/aarch64-linux-gnu/epdm_prev_rel" \
  ]

COPY --from=epdm [ \
  "/home/infinera/include/", \
  "/usr/local/include/aarch64-linux-gnu" \
  ]

COPY --from=milb_prev_rel [ \
  "/home/infinera/include/", \
  "/usr/local/include/aarch64-linux-gnu/milb_prev_rel" \
  ]

COPY --from=milb [ \
  "/home/infinera/include/", \
  "/usr/local/include/aarch64-linux-gnu" \
  ]

# Get chm6 logging
COPY --from=chm6_logging_arm64 [ \
  "/home/infinera/lib/aarch64/libInfnLogger.a", \
  "/usr/local/lib" \
  ]

COPY --from=fdrlib_ms_arm64 [ \
  "/home/infinera/build/aarch64/lib/libfdradapter.a", \
  "/usr/lib/aarch64-linux-gnu/" \
  ]

COPY --from=fdrlib_ms_arm64 [ \
  "/home/infinera/build/aarch64/python/RedisIfReflectionCodeGeneration.py", \
  "/home/infinera/src/" \
  ]

COPY --from=fdrlib_ms_arm64 [ \
  "/home/infinera/build/aarch64/python/fdr_mapping.csv", \
  "/home/infinera/src/" \
  ]


#Compile
# Cross compile as BUILDER here
WORKDIR /home/infinera/build

# hadolint ignore=SC2044,SC2006,SC2061,SC2035,DL3003,DL4006
RUN cd /home/infinera/src_gen/cpp/ && \
header="chm6PbHeader.h" && \
echo "" > $header && \
for each in `find . -name *.pb.h`; do h=`echo "$each"| sed 's@\./@@g'` ; echo "#include <$h>" >> $header; done

WORKDIR /home/infinera/build

RUN python3 /home/infinera/src/RedisIfReflectionCodeGeneration.py -o /home/infinera/src_gen/cpp/compiled_generation.cc -f  /home/infinera/src/fdr_mapping.csv -m chm6_dataplane_ms


#note: can use this to debug compilation issues by running compilation manually, comment out once done
#FROM chm6_dp_ms_prebuild_aarch64 as chm6_build_dbg_arm64

# hadolint ignore=SC2046,SC2061,SC2035
RUN cmake -DCI=OFF -DCMAKE_TOOLCHAIN_FILE="../Toolchain-arm.cmake" ../ -DARCH:STRING=arm64

#-------------------------Build aarch64 -----------------------------------#
FROM chm6_dp_ms_prebuild_aarch64 as chm6_dp_ms_build_aarch64

RUN make -j$(nproc) VERBOSE=1
WORKDIR /home/infinera/build/bin
RUN aarch64-linux-gnu-strip chm6_dp_prev_rel_ms -o chm6_dp_prev_rel_ms.stripped
RUN aarch64-linux-gnu-strip chm6_dp_ms -o chm6_dp_ms.stripped

#----------------------------- ARM64 Signing stage-----------------------------------------#
FROM signingkit:${signingkit_version} as chm6_dp_ms_build_aarch64_signed

COPY --from=chm6_dp_ms_build_aarch64 /home/infinera/build/bin/chm6_dp_prev_rel_ms.stripped /signing_mount
COPY --from=chm6_dp_ms_build_aarch64 /home/infinera/build/bin/chm6_dp_ms.stripped /signing_mount
COPY --from=chm6_dp_ms_build_aarch64 /home/infinera/build/bin/chm6_dp_prev_rel_ms /signing_mount
COPY --from=chm6_dp_ms_build_aarch64 /home/infinera/build/bin/chm6_dp_ms /signing_mount
COPY ./src/serdesTunerManager/CHM6_Loss.csv /signing_mount
COPY ./src/serdesTunerManager/CHM6_LUT.csv /signing_mount
# hadolint ignore=SC2154,SC2086
RUN /sign.py --sign-filesystem -w /signing_mount -c /signConfig.txt --action=${signing_action} -a arm64

#------------------------------ ARM64 SIGNED RUNTIME IMAGE --------------------------------------
FROM sv-artifactory.infinera.com/infnbase/arm64/chm6runenv:${chm6runenv}-signed as chm6_dataplane_ms_arm64_signed


RUN mkdir -p /home/bin
COPY --from=chm6_dp_ms_build_aarch64_signed [ \
    "/signing_mount/chm6_dp_prev_rel_ms.stripped", \
    "/home/bin/chm6_dp_prev_rel_ms" \
    ]
COPY --from=chm6_dp_ms_build_aarch64_signed [ \
    "/signing_mount/chm6_dp_ms.stripped", \
    "/home/bin/chm6_dp_ms" \
    ]
COPY --from=chm6_dp_ms_build_aarch64_signed [ \
    "/signing_mount/CHM6_Loss.csv", \
    "/signing_mount/CHM6_LUT.csv", \
    "/var/local/" \
    ]

COPY scripts/start_dp_ms.sh /home/bin/
RUN chmod +x /home/bin/start_dp_ms.sh

WORKDIR /home/bin/

FROM sv-artifactory.infinera.com/infnbase/arm64/chm6runenv:${chm6runenv}-signed as chm6_dataplane_ms_arm64_sym_signed

RUN mkdir -p /home/bin
COPY --from=chm6_dp_ms_build_aarch64_signed [ \
    "/signing_mount/chm6_dp_prev_rel_ms", \
    "/home/bin/" \
    ]
COPY --from=chm6_dp_ms_build_aarch64_signed [ \
    "/signing_mount/chm6_dp_ms", \
    "/home/bin/" \
    ]
COPY --from=chm6_dp_ms_build_aarch64_signed [ \
    "/signing_mount/CHM6_Loss.csv", \
    "/signing_mount/CHM6_LUT.csv", \
    "/var/local/" \
    ]

WORKDIR /home/bin/
#------------------------------RUNTIME IMAGE-----------------------------------------#
FROM sv-artifactory.infinera.com/infnbase/arm64/chm6runenv:${chm6runenv} as chm6_dataplane_ms_arm64

ARG artifact_root_dir
ARG arch=aarch64

RUN mkdir -p /home/bin
COPY --from=chm6_dp_ms_build_aarch64 [ \
    "/home/infinera/build/bin/chm6_dp_prev_rel_ms.stripped", \
    "/home/bin/chm6_dp_prev_rel_ms" \
    ]
COPY --from=chm6_dp_ms_build_aarch64 [ \
    "/home/infinera/build/bin/chm6_dp_ms.stripped", \
    "/home/bin/chm6_dp_ms" \
    ]

COPY ./src/serdesTunerManager/CHM6_Loss.csv /var/local/
COPY ./src/serdesTunerManager/CHM6_LUT.csv /var/local/

COPY scripts/start_dp_ms.sh /home/bin/
RUN chmod +x /home/bin/start_dp_ms.sh

WORKDIR /home/bin/

FROM sv-artifactory.infinera.com/infnbase/arm64/chm6runenv:${chm6runenv} as chm6_dataplane_ms_arm64_sym

COPY scripts/start_dp_ms.sh /home/bin/
RUN chmod +x /home/bin/start_dp_ms.sh

RUN mkdir -p /home/bin
COPY --from=chm6_dp_ms_build_aarch64 [ \
    "/home/infinera/build/bin/chm6_dp_prev_rel_ms", \
    "/home/bin/" \
    ]
COPY --from=chm6_dp_ms_build_aarch64 [ \
    "/home/infinera/build/bin/chm6_dp_ms", \
    "/home/bin/" \
    ]

WORKDIR /home/bin/

#-------------------------Prebuild x86_64 -----------------------------------#
FROM chm6_dp_ms_dev as chm6_dp_ms_prebuild_x86
ARG artifact_root_dir
ARG arch=x86_64

# Add common modules
RUN mkdir -p /home/src/include

COPY --from=chm6_common_x86 [ \
  "/home/src/build/x86_64/include/gecko_include", \
  "/home/src/include" \
  ]

COPY --from=chm6_common_x86 [ \
  "/home/src/build/x86_64/include", \
  "/home/src/include" \
  ]
COPY --from=chm6_common_x86 [ \
  "/home/src/build/x86_64/lib", \
  "/usr/local/lib" \
  ]

COPY --from=chm6_common_x86 [ \
  "/home/src/build/x86_64/include/keymgmt_include", \
  "/home/src/include" \
  ]

COPY --from=chm6_serdes_x86 [ \
  "/home/src/build/x86_64/include", \
  "/home/src/include" \
  ]
COPY --from=chm6_serdes_x86 [ \
  "/home/src/build/x86_64/lib", \
  "/usr/local/lib" \
  ]
  COPY --from=ds_apps_adapter_x86_64 [ \
  "/home/infinera/build/x86_64/lib/*", \
  "/usr/local/lib/" \
  ]

COPY --from=ds_apps_adapter_x86_64 [ \
  "/home/infinera/build/x86_64/include", \
  "/usr/local/include/" \
  ]

COPY --from=opentracing [ \
  "${artifact_root_dir}/${arch}-linux-gnu/lib", \
  "/usr/lib/${arch}-linux-gnu/" \
]

COPY --from=opentracing [ \
  "${artifact_root_dir}/${arch}-linux-gnu/include", \
  "/usr/local/include/${arch}-linux-gnu/" \
]

COPY --from=lightstep [ \
  "${artifact_root_dir}/${arch}-linux-gnu/lib", \
  "/usr/lib/${arch}-linux-gnu/" \
]

COPY --from=lightstep [ \
  "${artifact_root_dir}/${arch}-linux-gnu/include", \
  "/usr/local/include/${arch}-linux-gnu/" \
]

COPY --from=tracer_x86_64 [ \
  "${artifact_root_dir}/lib/${arch}-linux-gnu/", \
  "/usr/lib/${arch}-linux-gnu/" \
]

COPY --from=tracer_x86_64 [ \
  "${artifact_root_dir}/include/${arch}-linux-gnu/", \
  "/usr/local/include/${arch}-linux-gnu/" \
]

# Copy GRPC_adapter client lib files
COPY --from=grpc_adapter_x86 [ \
  "/home/infinera/lib/x86/libchm6grpcadapter.a", \
  "/usr/local/lib/" \
  ]

#get gnmi static lib
COPY --from=gnmi_lib [ \
  "/home/Infinera/gnmi_gen/build/x86/lib/libgnmi.a", \
  "/usr/local/lib/" \
  ]

COPY --from=epdm [ \
  "/home/infinera/include/", \
  "/usr/local/include/x86_64-linux-gnu" \
  ]

COPY --from=milb [ \
  "/home/infinera/include/", \
  "/usr/local/include/x86_64-linux-gnu" \
  ]


# Get json cpp for c++ desrialization
COPY --from=jsoncpp_chm6 [ \
  "/home/infinera/lib_jsoncpp/x86/libjsoncpp.a", \
  "/usr/local/lib/" \
 ]

# Get chm6 logging
COPY --from=chm6_logging_x86 [ \
  "/home/infinera/lib/x86_64/libInfnLogger.a", \
  "/usr/local/lib" \
  ]

COPY --from=fdrlib_ms_x86 [ \
  "/home/infinera/build/x86_64/lib/libfdradapter.a", \
  "/usr/local/lib/" \
  ]

COPY --from=fdrlib_ms_x86 [ \
  "/home/infinera/build/x86_64/python/RedisIfReflectionCodeGeneration.py", \
  "/home/infinera/src/" \
  ]

COPY --from=fdrlib_ms_x86 [ \
  "/home/infinera/build/x86_64/python/fdr_mapping.csv", \
  "/home/infinera/src/" \
  ]



RUN apt-get install -y --no-install-recommends libboost-log-dev:amd64=1.67.0.1

# Building src
WORKDIR /home/infinera/build

# hadolint ignore=SC2044,SC2006,SC2061,SC2035,DL3003,DL4006
RUN cd /home/infinera/src_gen/cpp/ && \
header="chm6PbHeader.h" && \
echo "" > $header && \
for each in `find . -name *.pb.h`; do h=`echo "$each"| sed 's@\./@@g'` ; echo "#include <$h>" >> $header; done

WORKDIR /home/infinera/build

RUN python3 /home/infinera/src/RedisIfReflectionCodeGeneration.py -o /home/infinera/src_gen/cpp/compiled_generation.cc -f  /home/infinera/src/fdr_mapping.csv -m chm6_dataplane_ms


#todo: can use this to debug build by running manually, comment out once done
#FROM chm6_dp_ms_build_x86 as chm6_build_dbg_x86

# hadolint ignore=SC2046,SC2061,SC2035
RUN cmake -DCI=OFF -DARCH:STRING=x86 ..

#-------------------------Build x86_64 -----------------------------------#
FROM chm6_dp_ms_prebuild_x86 as chm6_dp_ms_build_x86

RUN make -j$(nproc)
WORKDIR /home/infinera/build/bin
RUN x86_64-linux-gnu-strip chm6_dp_ms -o chm6_dp_ms.stripped
COPY ./src/serdesTunerManager/CHM6_Loss.csv /var/local/
COPY ./src/serdesTunerManager/CHM6_LUT.csv /var/local/

#-----------------------------x86 Signing stage-----------------------------------------#
FROM signingkit:${signingkit_version} as chm6_dp_ms_build_x86_signed

COPY --from=chm6_dp_ms_build_x86 /home/infinera/build/bin/chm6_dp_ms.stripped /signing_mount
COPY --from=chm6_dp_ms_build_x86 /home/infinera/build/bin/chm6_dp_ms /signing_mount
COPY ./src/serdesTunerManager/CHM6_Loss.csv /signing_mount
COPY ./src/serdesTunerManager/CHM6_LUT.csv /signing_mount
# hadolint ignore=SC2154,SC2086
RUN /sign.py --sign-filesystem -w /signing_mount -c /signConfig.txt --action=${signing_action}

#-------------------------------x86 SIGNED RUNTIME IMAGE-------------------------------
FROM sv-artifactory.infinera.com/infnbase/x86_64/chm6runenv:${chm6runenv}-signed as chm6_dataplane_ms_x86_signed

RUN mkdir -p /home/bin
COPY scripts/start_dp_ms.sh /home/bin/
RUN chmod +x /home/bin/start_dp_ms.sh

WORKDIR /home
RUN mkdir -p /home/bin
COPY --from=chm6_dp_ms_build_x86_signed [ \
    "/signing_mount/chm6_dp_ms.stripped", \
    "/home/bin/chm6_dp_ms" \
    ]
COPY --from=chm6_dp_ms_build_x86_signed [ \
    "/signing_mount/CHM6_Loss.csv", \
    "/signing_mount/CHM6_LUT.csv", \
    "/var/local/" \
    ]

WORKDIR /home/bin/

FROM sv-artifactory.infinera.com/infnbase/x86_64/chm6runenv:${chm6runenv}-signed as chm6_dataplane_ms_x86_sym_signed

RUN mkdir -p /home/bin
COPY scripts/start_dp_ms.sh /home/bin/
RUN chmod +x /home/bin/start_dp_ms.sh

WORKDIR /home
RUN mkdir -p /home/bin
COPY --from=chm6_dp_ms_build_x86_signed [ \
    "/signing_mount/chm6_dp_ms", \
    "/home/bin/" \
    ]
COPY --from=chm6_dp_ms_build_x86_signed [ \
    "/signing_mount/CHM6_Loss.csv", \
    "/signing_mount/CHM6_LUT.csv", \
    "/var/local/" \
    ]

WORKDIR /home/bin/

#-------------------------------x86 RUNTIME IMAGE-------------------------------
FROM sv-artifactory.infinera.com/infnbase/x86_64/chm6runenv:${chm6runenv} as chm6_dataplane_ms_x86

ARG artifact_root_dir
ARG arch=x86_64

RUN mkdir -p /home/bin
COPY scripts/start_dp_ms.sh /home/bin/
RUN chmod +x /home/bin/start_dp_ms.sh

WORKDIR /home
RUN mkdir -p /home/bin
COPY --from=chm6_dp_ms_build_x86 [ \
    "/home/infinera/build/bin/chm6_dp_ms.stripped", \
    "/home/bin/chm6_dp_ms" \
    ]

WORKDIR /home/bin/

COPY ./src/serdesTunerManager/CHM6_Loss.csv /var/local/
COPY ./src/serdesTunerManager/CHM6_LUT.csv /var/local/

FROM sv-artifactory.infinera.com/infnbase/x86_64/chm6runenv:${chm6runenv} as chm6_dataplane_ms_x86_sym

WORKDIR /home
RUN mkdir -p /home/bin
COPY --from=chm6_dp_ms_build_x86 [ \
    "/home/infinera/build/bin/chm6_dp_ms", \
    "/home/bin/" \
    ]

WORKDIR /home/bin/

#-------------------------x86_64 static-analysis------------------------------#
FROM chm6_dp_ms_prebuild_x86 as static_analysis_setup

COPY --from=baseimg /opt/infinera /opt/infinera
COPY --from=baseimg /opt/Coverity /opt/Coverity
COPY --from=baseimg /home/infinera /home/infinera
COPY --from=baseimg /usr/local/lib/python3.7 /usr/local/lib/python3.7

RUN mkdir -p /home/infinera/artifacts/x86_64-linux-gnu/static_analysis

#-------------------------x86_64 static-analysis------------------------------#
FROM static_analysis_setup as static_analysis
ARG arch=x86_64
ARG static_analysis_args
ARG artifact_root_dir=/home/infinera/artifacts

RUN /home/infinera/bin/coverity \
  --json_args "${static_analysis_args}" \
  --src_root_dir "/home/infinera/" \
  --src_build_dir "/home/infinera/build" \
  --src_build_cmd "make -j\"$(nproc)\" VERBOSE=1" \
  --coverity_json_out_file "${artifact_root_dir}/${arch}-linux-gnu/static_analysis/chm6_dataplane_ms.json" \
  --coverity_html_out_dir "${artifact_root_dir}/${arch}-linux-gnu/static_analysis/html"
