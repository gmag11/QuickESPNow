/**
  * @file Comms_hal.h
  * @version 0.9.8
  * @date 15/07/2021
  * @author German Martin
  * @brief Generic communication system abstraction layer
  *
  * This is the interface that communication definition should implement to be used as layer 1 on EnigmaIoT
  */
#ifndef _COMMS_HAL_h
#define _COMMS_HAL_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

//typedef void (*comms_hal_rcvd_data)(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);
typedef std::function<void (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast)> comms_hal_rcvd_data;
//typedef void (*comms_hal_sent_data)(uint8_t* address, uint8_t status);
typedef std::function<void (uint8_t* address, uint8_t status)> comms_hal_sent_data;

/**
  * @brief Interface for communication subsystem abstraction layer definition
  */
class Comms_halClass {
protected:
	//uint8_t gateway[ESPNOW_ADDR_LEN]; ///< @brief Gateway address
	uint8_t channel; ///< @brief Comms channel to be used

	comms_hal_rcvd_data dataRcvd = 0; ///< @brief Pointer to a function to be called on every received message
	comms_hal_sent_data sentResult = 0; ///< @brief Pointer to a function to be called to notify last sending status
	//peerType_t _ownPeerType; ///< @brief Stores peer type, node or gateway

	/**
	  * @brief Communication subsistem initialization
	  * @param peerType Role that peer plays into the system, node or gateway.
	  */
	virtual void initComms () = 0;


public:
    Comms_halClass () {}
    
	/**
	  * @brief Setup communication environment and establish the connection from node to gateway
	  * @param gateway Address of gateway. It may be `NULL` in case this is used in the own gateway
	  * @param channel Establishes a channel for the communication. Its use depends on actual communications subsystem
	  * @param peerType Role that peer plays into the system, node or gateway.
	  */
    virtual bool begin (uint8_t channel, uint32_t interface = 0) = 0;

	/**
	  * @brief Terminates communication and closes all connectrions
	  */
	virtual void stop () = 0;

	/**
	  * @brief Sends data to the other peer
	  * @param da Destination address to send the message to
	  * @param data Data buffer that contain the message to be sent
	  * @param len Data length in number of bytes
	  * @return Returns sending status. 0 for success, any other value to indicate an error.
	  */
	virtual int32_t send (const uint8_t* da, const uint8_t* data, size_t len) = 0;

	/**
	  * @brief Attach a callback function to be run on every received message
	  * @param dataRcvd Pointer to the callback function
	  */
	virtual void onDataRcvd (comms_hal_rcvd_data dataRcvd) = 0;

	/**
	  * @brief Attach a callback function to be run after sending a message to receive its status
	  * @param dataRcvd Pointer to the callback function
	  */
	virtual void onDataSent (comms_hal_sent_data dataRcvd) = 0;

	/**
	  * @brief Get address length that a specific communication subsystem uses
	  * @return Returns number of bytes that is used to represent an address
	  */
    virtual uint8_t getAddressLength () = 0;

    /**
      * @brief Get max message length for a specific communication subsystems
      * @return Returns number of bytes of longer supported message
      */
    virtual uint8_t getMaxMessageLength () = 0;

    /**
      * @brief Enables or disables transmission of queued messages. Used to disable communication during wifi scan
      * @param enable `true` to enable transmission, `false` to disable it
      */
    virtual void enableTransmit (bool enable) = 0;
};

#endif

