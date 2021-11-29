# LoRaWAN ns-3 Class B module Extension #

Extending on an already existing [Class-A simulator](https://github.com/signetlabdei/lorawan),
this module extends it to include LoRaWAN Class B multicast.

## What is in the module ##
* Creation of multicast groups and analyzing beacon and downlink related performance.
* Simulation of LoRaWAN Class B unicast using multicast groups with single member.
* According to LoRaWAN Class B specification.
  - Beaconless operation mode is implimented.
  - Ping-slot randomization is included.
* There is also an option to enable a technique called "ping slot relaying" which is not part of the standard but added for research purpose.

**Note**: "Ping slot relaying" is a techique devoped as part of the LoRaWAN Class B multicast scalability study to improve the multicast performance without using the gateways. (See publication in the "Acknowledgments and relevant publications" section)

## What is not in the module ##
* Crystal clock inaccuracy has not been modeled. 
* Device address is assigned in increamental order, hence random address assignenet is not done yet.
* Co-existance of Class A and Class B has not been resolved in the Network Server; hence you can test one at a time.
* Multiple gateway situation has not been fully tested yet.

## TO DO ##
* Doing the remaining work in "What is not in the module" section. 
* Merging with the [Class-A simulator](https://github.com/signetlabdei/lorawan).
* Editing `README.md`.
* Updating documentation.

## Detailed Architecture and documentation ##

The detailed architecture and the discription of the module including the limitations of the module are detailed in Chapter 3 of the LoRaWAN Class B multicast: Scalablity (Master thesis). (See publication in the "Acknowledgments and relevant publications" section)

## Author ##
 - Yonatan Shiferaw

## Prerequisite and building 

Since this module is written for version 3.29 it does not work with the latest branch. You can build for version 3.29 as follows. 

```
wget https://www.nsnam.org/releases/ns-allinone-3.29.tar.bz2

tar -xvf ns-allinone-3.29.tar.bz2

cd ns-allinone-3.29/

cd ns-3.29/

git clone https://github.com/yoniwt/lorawan-private.git src/lorawan

./waf configure --enable-tests --enable-examples

./waf build
```

## Creating a directory for logging (optional) 

The examples generate a log file and you can create the directories as follows in `ns-29/` directory for the log files. 

```
 mkdir ClassBResults

 mkdir ClassBResults/MulticastGroupPerformance/

 mkdir ClassBResults/Basic/

 mkdir ClassBResults/BeaconBlocking
```


## Usage examples ##

The module includes the following examples:

- `class-b-network-example`
- `class-b-network-example-beacon-performance`
- `class-b-network-example-multicast-performance`

Examples can be run via the `./waf --run example-name` command.
See the comment on the top of the script for more detail and arguments to use.


## License ##

This software is licensed under the terms of the GNU GPLv2 (the same license
that is used by ns-3). See the LICENSE.md file for more details.

## Acknowledgments and relevant publications ##

The LoRaWAN Class B multicast module extention was developed as part of a master's thesis at
the [Technical University of Delft](https://www.tudelft.nl "TU Delft homepage"), under the
supervision [Dr. Ir. Fernando Kuipers](https://fernandokuipers.nl/).

Publications:
- LoRaWAN Class B multicast: Scalablity (Master thesis).
  https://repository.tudelft.nl/islandora/object/uuid:09af41b0-28ec-40f2-b858-4cbd75edae0c?collection=education
- Y. Shiferaw, A. Arora, and F. Kuipers. LoRaWAN Class B Multicast Scalability. In IFIP Networking Conference, pages 609–613, Jun 2020.
  https://ieeexplore.ieee.org/document/9142813
