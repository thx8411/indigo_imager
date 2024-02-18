// Copyright (c) 2024 Blaise-Florentin Collin
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//#include "imagerwindow.h"
//#include "propertycache.h"
//#include "conf.h"
//#include "widget_state.h"
//#include "utils.h"

#include <imagerwindow.h>
#include <propertycache.h>
#include <indigoclient.h>
#include <conf.h>
#include <logger.h>

void write_conf();

void ImagerWindow::create_map_tab(QFrame *map_frame) {
	QSpacerItem *spacer;

	QGridLayout *map_frame_layout = new QGridLayout();
	map_frame_layout->setAlignment(Qt::AlignTop);
	//map_frame_layout->setColumnStretch(0, 1);
	//map_frame_layout->setColumnStretch(1, 1);
	//map_frame_layout->setColumnStretch(2, 1);
	//map_frame_layout->setColumnStretch(3, 1);

	map_frame->setLayout(map_frame_layout);
	map_frame->setFrameShape(QFrame::StyledPanel);
	map_frame->setMinimumWidth(CAMERA_FRAME_MIN_WIDTH);
	map_frame->setContentsMargins(0, 0, 0, 0);

	int row = 0;

}
