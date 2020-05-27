/* 
 * File:   file_utils.h
 * Author: Jon's Windows
 *
 * Created on January 7, 2019, 1:11 PM
 */

#ifndef FILE_UTILS_H
#define	FILE_UTILS_H

#ifdef	__cplusplus
extern "C" {
#endif


void ls_command(char *args[]);
void cat_command(char *args[]);
void cat_command_continue();
void hyperterminal_command(char *args[]);
void stty_command(char *args[]);
void feedlog(char *msg);
void feedlog_always(char *msg);



#ifdef	__cplusplus
}
#endif

#endif	/* FILE_UTILS_H */

