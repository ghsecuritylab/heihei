#ifndef __TFTP_SERVER_OPTS_H
#define __TFTP_SERVER_OPTS_H

/**
 * Determine weather do the debug print
 */
#define TFTP_SERVER_DEBUG  1

/**
 * TFTP server port
 */
#define TFTP_SERVER_PORT             69

/**
 * TFTP server timeout
 */
#define TFTP_SERVER_TIMEOUT_MSECS    5000

/**
 * Max. number of retries when a file is read from server
 */
#define TFTP_SERVER_MAX_RETRIES      5

/**
 * TFTP server timer cyclic interval
 */
#define TFTP_SERVER_TIMER_MSECS      50

/**
 * Max. length of TFTP filename
 */
#define TFTP_SERVER_MAX_FILENAME_LEN 32

/**
 * Max. length of TFTP mode
 */
#define TFTP_SERVER_MAX_MODE_LEN     16

#endif
