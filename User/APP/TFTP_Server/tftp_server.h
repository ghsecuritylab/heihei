#ifndef __TFTP_SERVER_H
#define __TFTP_SERVER_H

#include "cc.h"
#include "lwip/err.h"


struct tftp_file_op {
  /**
   * Open file for read/write.
   * @param fname Filename
   * @param mode Mode string from TFTP RFC 1350 (netascii, octet, mail)
   * @param write Flag indicating read (0) or write (!= 0) access
   * @returns File handle supplied to other functions
   */
  void* (*open)(char* fname, char* mode, u8_t write);
  /**
   * Close file handle
   * @param handle File handle returned by open()
   */
  void (*close)(void* handle);
  /**
   * Read from file 
   * @param handle File handle returned by open()
   * @param buf Target buffer to copy read data to
   * @param bytes Number of bytes to copy to buf
   * @returns &gt;= 0: Success; &lt; 0: Error
   */
  int (*read)(void* handle, u8_t* buf, int bytes);
  /**
   * Write to file
   * @param handle File handle returned by open()
   * @param pbuf PBUF adjusted such that payload pointer points
   *             to the beginning of write data. In other words,
   *             TFTP headers are stripped off.
   * @returns &gt;= 0: Success; &lt; 0: Error
   */
  int (*write)(void* handle, u8_t *buf, int bytes);
};

err_t tftp_server_init(struct tftp_file_op *p);

#endif
