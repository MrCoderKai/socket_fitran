# socket_fitran
A demo for socket programming.

# Problems Encoutered in Development
## Access
When client want to access to the server, it must generate a temporary ID, namely `m_tRNTI`, as its access id. After receiving the access ack information from server, it should check is `m_tRNTI` whether this access ack information is its or not.

## Server Handles Multiple Client Access
