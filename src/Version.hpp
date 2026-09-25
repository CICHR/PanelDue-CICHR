/*
 * Version.hpp
 *
 *  Created on: 15 Mar 2018
 *      Author: David
*/

#ifndef SRC_VERSION_HPP_
#define SRC_VERSION_HPP_

#define VERSION_MAIN		"1.0.0"

#ifdef SUPPORT_ENCODER
# define VERSION_ENCODER	"+enc"
#else
# define VERSION_ENCODER
#endif

#ifdef DEVICE
# define STRINGIFY(x)	#x
# define TOSTRING(x)	STRINGIFY(x)
# define VERSION_DEVICE "-" TOSTRING(DEVICE)
#else
# define VERSION_DEVICE
#endif

#define VERSION_TEXT VERSION_MAIN VERSION_ENCODER VERSION_DEVICE
#ifdef DEVICE
# define SETTINGS_VERSION_TEXT "PanelDue-CICHR v" VERSION_MAIN " [" TOSTRING(DEVICE) "]"
#else
# define SETTINGS_VERSION_TEXT "PanelDue-CICHR v" VERSION_MAIN
#endif

#endif /* SRC_VERSION_HPP_ */
