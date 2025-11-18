**Description**

Software to configure and control RJ45 Timing Fanout (RTF) board.
- `lib`: contains Ethernet interface libraries
- `RTFController`: template C++ code to configure the board
- `EMPHATIC_RTF_SacredConfigs`: default running configurations for EMPHATIC operations [UNDER CONSTRUCTION]

**Integration workflow**

To add a new testing configuration: create a separated directory with a copy of `RTFControllerTemplate/otsEthTest.cpp`

**Current custom EMPHATIC setups**
- `RTFController_test111825`
  -  Update ip adress to use EMPHATIC private network
  -  Clock configuration: [TO BE FILLED]
  -  Trigger configuration [TO BE FILLED]
