/*
Copyright (C) 2011 MoSync AB

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License,
version 2, as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301, USA.
*/

/*! \addtogroup NativeUILib
 *  @{
 */

/**
 *  @defgroup NativeUILib Native UI Library
 *  @{
 */

/**
 * @file CameraPreview.h
 * @author Emma Tresanszki
 *
 * \brief An instance of CameraPreview allows users to visualize the camera controller.
 */

#ifndef NATIVEUI_CAMERAPREVIEW_H_
#define NATIVEUI_CAMERAPREVIEW_H_

#include "Widget.h"

namespace NativeUI
{
	/** \brief An instance of CameraPreview allows users to visualize the
	 * camera controller.
	 * Note: This is not available on Windows Phone 7.
	 *
	 * A Camera Preview is a widget that is used to display  a live view
	 * of the camera rendered by the viewfinder.
	 * It has no specific properties (besides the base widget properties)
	 * attached to it because the camera is mostly handled via syscalls.
	 * So this widget is only used to enable the camera controller to be
	 * mapped with UI in devices that support Native UI.
	 * After creating the widget, remember to bind it to the camera controller
	 * via a call to #maCameraSetPreview (passing the value from it's
	 * getWidgetHandle() method), or by calling the bindToCurrentCamera() method.
	 */
	class CameraPreview : public Widget
	{
	public:
		/**
		 * Constructor.
		 */
		CameraPreview();

		/**
         * Bind the preview to the currently selected camera.
		 * @return 1 on success or 0 for failure
         */
        virtual int bindToCurrentCamera();

		/**
		 * Destructor.
		 */
		virtual ~CameraPreview();

	};
} // namespace NativeUI

#endif /* NATIVEUI_CAMERAPREVIEW_H_ */

/*! @} */
