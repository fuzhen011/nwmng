/*************************************************************************
    > File Name: generic_parser.h
    > Author: Kevin
    > Created Time: 2019-12-18
    > Description:
 ************************************************************************/

#ifndef GENERIC_PARSER_H
#define GENERIC_PARSER_H
#ifdef __cplusplus
extern "C"
{
#endif

/*
 * String keys in both the network & nodes config file and the provisioner
 * config file
 */
#define STR_REFID                         "RefId"
#define STR_ID                            "Id"
#define STR_DONE                          "Done"
#define STR_CNT                           "Count"
#define STR_INTV                          "Interval"
#define STR_SUBNETS                       "Subnets"
#define STR_APPKEY                        "AppKey"
#define STR_TTL                           "TTL"

/*
 * String keys only in the provisioner config file
 */
#define STR_VALUE                         "Value"
#define STR_SYNC_TIME                     "SyncTime"
#define STR_IVI                           "IVI"

/*
 * String keys only in the network & nodes config file
 */
#define STR_NODES                         "Nodes"
#define STR_NODE                          "Node"
#define STR_UUID                          "UUID"
#define STR_FEATURES                      "Features"

/* TODO: Needs to clear */
#define STR_NETKEY                        "NetKey"
#define STR_KEY                           "Key"

#define STR_TEMPLATES                     "Templates"
#define STR_ERRBITS                       "Err"
#define STR_BL                            "Blacklist"
#define STR_TMPL                          "Tmpl"
#define STR_BIND                          "Bind"
#define STR_PUB_BIND                      "Pub_bind"
#define STR_SUB                           "Sub"
#define STR_TRANS_CNT                     "Trans"
#define STR_TRANS_INTERVAL                "Tran_interval"
#define STR_PERIOD                        "Period"

#define STR_SECURE_NETWORK_BEACON "SNB"
#define STR_LPN "LPN"
#define STR_PROXY "Proxy"
#define STR_FRIEND "Friend"
#define STR_RELAY "Relay"
#define STR_NET_RETRAN "Net_Retransmit"
#define STR_ENABLE "Enable"
#define STR_PUB                           "Pub"
/* TODO: Needs to clear */

#ifdef __cplusplus
}
#endif
#endif //GENERIC_PARSER_H
