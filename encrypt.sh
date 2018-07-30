#! /bin/bash
#openssl enc -aes-256-cbc -base64 -pass file:password -in msg -out enc_msg
gmssl sms4-cbc -base64 -pass file:password -in msg -out enc_msg