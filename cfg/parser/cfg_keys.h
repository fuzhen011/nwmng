/*************************************************************************
    > File Name: cfg_keys.h
    > Author: Kevin
    > Created Time: 2019-12-18
    > Description: Keys used in config files
 ************************************************************************/

#ifndef CFG_KEYS_H
#define CFG_KEYS_H
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
#define STR_ADDR                          "Address"
#define STR_DONE                          "Done"
#define STR_CNT                           "Count"
#define STR_INTV                          "Interval"
#define STR_SUBNETS                       "Subnets"
#define STR_APPKEY                        "AppKey"
#define STR_TTL                           "TTL"
#define STR_TXP                           "TX Parameters"

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
#define STR_BACKLOG                       "Backlog"
#define STR_NODE                          "Node"
#define STR_UUID                          "UUID"
#define STR_FEATURES                      "Features"
#define STR_RMORBL                        "RM_Blacklist"
#define STR_ERRBITS                       "Err"
#define STR_TMPL                          "Template ID"
#define STR_SNB                           "Secure Network Beacon"
#define STR_LPN                           "Low Power"
#define STR_PROXY                         "Proxy"
#define STR_FRIEND                        "Friend"
#define STR_RELAY                         "Relay"
#define STR_ENABLE                        "Enable"
#define STR_PUB                           "Publish To"
#define STR_BIND                          "Bind Appkeys"
#define STR_SUB                           "Subscribe from"
#define STR_PERIOD                        "Period"

/*
 * String keys only in the network & nodes config file
 */
#define STR_TEMPLATES                     "Templates"

#ifdef __cplusplus
}
#endif
#endif //CFG_KEYS_H
