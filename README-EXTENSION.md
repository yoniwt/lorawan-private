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

## Author ##
 - Yonatan Shiferaw

## Usage examples ##

The module includes the following examples:

- `class-b-network-example`
- `class-b-network-example-beacon-performance`
- `class-b-network-example-multicast-performance`

Examples can be run via the `./waf --run example-name` command.
See the comment on the top of the script for more detail.

## License ##

This software is licensed under the terms of the GNU GPLv2 (the same license
that is used by ns-3). See the LICENSE.md file for more details.

## Acknowledgments and relevant publications ##

The LoRaWAN Class B multicast module extention was developed as part of a master's thesis at
the [Technical University of Delft](https://www.tudelft.nl "TU Delft homepage"), under the
supervision [Dr. Ir. Fernando Kuipers](https://fernandokuipers.nl/).

Publications:
- LoRaWAN Class B multicast: Scalablity (Master thesis).  Embargo Until 2020-09-26:
  https://repository.tudelft.nl/islandora/object/uuid:09af41b0-28ec-40f2-b858-4cbd75edae0c?collection=education
