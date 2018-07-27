#! /bin/bash
openssl enc -aes-256-cbc -d -base64 -pass file:password -in enc_msg -out dec_msg
#gmssl sms4-cbc -d -base64 -pass file:password -in enc_msg -out dec_msg