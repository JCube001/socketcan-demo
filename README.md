# SocketCAN Demo Programs

These programs demonstrate how to use the SocketCAN API on Linux. You are free
to use these as a starting point for writing your own SocketCAN enabled
applications.

## Raw Interface Demo

This program demonstrates reading and writing to a CAN bus using SocketCAN's
Raw interface. The intended behavior of this program is to read in any CAN
message from the bus, add one to the value of each byte in the received
message, and then write that message back out on to the bus with the message ID
defined by the macro MSGID.

## Broadcast Manager Interface Demo

This program demonstrates reading and writing to a CAN bus using SocketCAN's
Broadcast Manager interface. The intended behavior of this program is to read
in CAN messages which have an ID of 0x123, add one to the value of each data
byte in the received message, and then write that message back out on to the
bus with the message ID defined by the macro MSGID.

