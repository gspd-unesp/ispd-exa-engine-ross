# ADD
## Allocator
Returns the machine that will receive an vm to host following certain criteria.
##  Configuration/virtual machine
Adds the configuration of a virtual machine in the simulation.

## metrics/virtual machine metrics
Encapsulates metrics and information related to the virtual machine

## services/virtual machine
Receive the task, process it and send an acknowledgment message to VMM. This will be changed later to support the new workload idea.

## services/VMM
Send the vms to be allocated in machines and once the allocation is finished send tasks to virtual machines.

# CHANGES
## Configuration/machine
Added information such as:
* available memory
* available disk space
* prices (memory/gb, disk/gb and core/gb)
* 
## Message
Added virtual machine information and new flags

## Machine metrics
Updated to have the total cost and the amount of allocated machines.

## metrics 
New metrics such as total allocated vms and costs.

## builder
Support virtual machine and virtual machine manager and also changed to support the new machine configuration

## services/machine
Forward the message to the virtual machine hosted in the machine or allocate a virtual machine if the VMM sent one.

# TODO

## New workload
Should be created another workload for the IAAS environment to model applications containing internal tasks (https://sol.sbc.org.br/index.php/wscad/article/view/21946)

## Polish the code
There are some gambiarras and functionality that could be done better (and will)

## Solve bugs
