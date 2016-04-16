#!/usr/bin/env bash
protoc -I=../protofiles --cpp_rpcz_out=. ../protofiles/$1
