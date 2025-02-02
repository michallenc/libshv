#include "graphwidget.h"
#include "graphmodel.h"
#include "graphview.h"
#include "channelprobewidget.h"


#include <shv/core/exception.h>
#include <shv/coreqt/log.h>

#include <QApplication>
#include <QPainter>
#include <QFontMetrics>
#include <QLabel>
#include <QMouseEvent>
#include <QToolTip>
#include <QDateTime>
#include <QMenu>
#include <QScreen>
#include <QScrollBar>
#include <QWindow>
#include <QDrag>
#include <QMimeData>
#include <cmath>
#include <QDesktopWidget>

#define logMouseSelection() nCDebug("MouseSelection")

namespace cp = shv::chainpack;

namespace shv {
namespace visu {
namespace timeline {

GraphWidget::GraphWidget(QWidget *parent)
	: Super(parent)
	, m_channelHeaderMoveContext(nullptr)
{
	setMouseTracking(true);
	setContextMenuPolicy(Qt::DefaultContextMenu);
}

void GraphWidget::setGraph(Graph *g)
{
	if(m_graph)
		m_graph->disconnect(this);
	m_graph = g;
	Graph::Style style = graph()->style();
	style.init(this);
	graph()->setStyle(style);
	update();
	connect(m_graph, &Graph::presentationDirty, this, [this](const QRect &rect) {
		//shvInfo() << "presentsation dirty:" << Graph::rectToString(rect);
		if(rect.isEmpty())
			update();
		else
			update(rect);
	});
	connect(m_graph, &Graph::layoutChanged, this, [this]() {
		makeLayout();
	});
	connect(m_graph, &Graph::graphContextMenuRequest, this, &GraphWidget::showGraphContextMenu);
	connect(m_graph, &Graph::channelContextMenuRequest, this, &GraphWidget::showChannelContextMenu);
}

Graph *GraphWidget::graph()
{
	return m_graph;
}

const Graph *GraphWidget::graph() const
{
	return m_graph;
}

void GraphWidget::setTimeZone(const QTimeZone &tz)
{
	graph()->setTimeZone(tz);
}

void GraphWidget::makeLayout(const QSize &preferred_size)
{
	shvLogFuncFrame();
	m_graphPreferredSize = preferred_size;
	graph()->makeLayout(QRect(QPoint(), preferred_size));
	QSize sz = graph()->rect().size();
	shvDebug() << "new size:" << sz.width() << 'x' << sz.height();
	if(sz.width() > 0)
		setMinimumSize(sz);
	update();
}

void GraphWidget::makeLayout()
{
	makeLayout(m_graphPreferredSize);
}

bool GraphWidget::event(QEvent *ev)
{
	if(Graph *gr = graph())
		gr->processEvent(ev);
	return Super::event(ev);
}

void GraphWidget::paintEvent(QPaintEvent *event)
{
	Super::paintEvent(event);
	auto *view_port = parentWidget();
	auto *scroll_area = qobject_cast<GraphView*>(view_port->parentWidget());
	if(!scroll_area) {
		shvError() << "Cannot find GraphView";
		return;
	}
	//auto rect_to_string = [](const QRect &r) {
	//	QString s = "%1,%2 %3x%4";
	//	return s.arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
	//};
	QPainter painter(this);
	const QRect dirty_rect = event->rect();
	//QRect view_rect = view_port->geometry();
	//view_rect.moveTop(-geometry().y());
	//shvInfo() << "-----------------------------------" << view_port->objectName();
	//shvInfo() << "dirty rect:"  << rect_to_string(dirty_rect);
	//shvInfo() << "view port :"  << rect_to_string(view_port->geometry());
	//shvInfo() << "widget    :"  << rect_to_string(geometry());
	//shvInfo() << "view_rect :"  << rect_to_string(view_rect);
	graph()->draw(&painter, dirty_rect,  scroll_area->widgetViewRect());
}

/*
void GraphWidget::keyPressEvent(QKeyEvent *event)
{
	shvDebug() << event->key();
	//if(event->key() == Qt::)
	Super::keyPressEvent(event);
}

void GraphWidget::keyReleaseEvent(QKeyEvent *event)
{
	shvLogFuncFrame();
	Super::keyReleaseEvent(event);
}
*/

bool GraphWidget::isMouseAboveMiniMap(const QPoint &mouse_pos) const
{
	const Graph *gr = graph();
	return gr->miniMapRect().contains(mouse_pos);
}

bool GraphWidget::isMouseAboveMiniMapHandle(const QPoint &mouse_pos, bool left) const
{
	const Graph *gr = graph();
	if(mouse_pos.y() < gr->miniMapRect().top() || mouse_pos.y() > gr->miniMapRect().bottom())
		return false;
	int x = left
			? gr->miniMapTimeToPos(gr->xRangeZoom().min)
			: gr->miniMapTimeToPos(gr->xRangeZoom().max);
	int w = gr->u2px(0.5);
	int x1 = left ? x - w : x - w/2;
	int x2 = left ? x + w/2 : x + w;
	return mouse_pos.x() > x1 && mouse_pos.x() < x2;
}

bool GraphWidget::isMouseAboveLeftMiniMapHandle(const QPoint &pos) const
{
	return isMouseAboveMiniMapHandle(pos, true);
}

bool GraphWidget::isMouseAboveRightMiniMapHandle(const QPoint &pos) const
{
	return isMouseAboveMiniMapHandle(pos, false);
}

bool GraphWidget::isMouseAboveMiniMapSlider(const QPoint &pos) const
{
	const Graph *gr = graph();
	if(pos.y() < gr->miniMapRect().top() || pos.y() > gr->miniMapRect().bottom())
		return false;
	int x1 = gr->miniMapTimeToPos(gr->xRangeZoom().min);
	int x2 = gr->miniMapTimeToPos(gr->xRangeZoom().max);
	return (x1 < pos.x()) && (pos.x() < x2);
}

int GraphWidget::posToChannelVerticalHeader(const QPoint &pos) const
{
	const Graph *gr = graph();
	for (int i = 0; i < gr->channelCount(); ++i) {
		const GraphChannel *ch = gr->channelAt(i);
		if(ch->verticalHeaderRect().contains(pos)) {
			return i;
		}
	}
	return -1;
}

int GraphWidget::posToChannel(const QPoint &pos) const
{
	const Graph *gr = graph();
	int ch_ix = gr->posToChannel(pos);
	return ch_ix;
}

void GraphWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();
	if(posToChannel(pos) >= 0) {
		if(event->modifiers() == Qt::NoModifier) {
			emit graphChannelDoubleClicked(pos);
			event->accept();
			return;
		}
	}
	Super::mouseDoubleClickEvent(event);
}

void GraphWidget::mousePressEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();
	if(event->button() == Qt::LeftButton) {
		if(isMouseAboveLeftMiniMapHandle(pos)) {
			m_mouseOperation = MouseOperation::MiniMapLeftResize;
			shvDebug() << "LEFT resize";
			event->accept();
			return;
		}
		else if(isMouseAboveRightMiniMapHandle(pos)) {
			m_mouseOperation = MouseOperation::MiniMapRightResize;
			event->accept();
			return;
		}
		else if(isMouseAboveMiniMapSlider(pos)) {
			m_mouseOperation = MouseOperation::MiniMapScrollZoom;
			m_recentMousePos = pos;
			event->accept();
			return;
		}
		else if (isMouseAboveChannelResizeHandle(pos)) {
			m_mouseOperation = MouseOperation::ChannelHeaderResize;
			m_resizeChannelIx = graph()->posToChannelHeader(pos);
			m_recentMousePos = pos;
			event->accept();
			return;
		}
		else if(posToChannel(pos) >= 0) {
			if(event->modifiers() == Qt::ControlModifier) {
				m_mouseOperation = MouseOperation::GraphDataAreaLeftCtrlPress;
			}
			else if(event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
				m_mouseOperation = MouseOperation::GraphDataAreaLeftCtrlShiftPress;
			}
			else {
				logMouseSelection() << "GraphAreaPress";
				m_mouseOperation = MouseOperation::GraphDataAreaLeftPress;
			}
			m_recentMousePos = pos;
			event->accept();
			return;
		}
		else if (posToChannelVerticalHeader(pos) > -1) {
			if (event->modifiers() == Qt::NoModifier) {
				m_mouseOperation = MouseOperation::ChannelHeaderMove;
			}
			m_recentMousePos = pos;
			event->accept();
			return;
		}
	}
	else if(event->button() == Qt::RightButton) {
		if(posToChannel(pos) >= 0) {
			if(event->modifiers() == Qt::NoModifier) {
				logMouseSelection() << "GraphAreaRightPress";
				m_mouseOperation = MouseOperation::GraphDataAreaRightPress;
				m_recentMousePos = pos;
				event->accept();
				return;
			}
		}
	}
	Super::mousePressEvent(event);
}

void GraphWidget::mouseReleaseEvent(QMouseEvent *event)
{
	logMouseSelection() << "mouseReleaseEvent, button:" << event->button() << "op:" << (int)m_mouseOperation;
	auto old_mouse_op = m_mouseOperation;
	m_mouseOperation = MouseOperation::None;
	if(event->button() == Qt::LeftButton) {
		if(old_mouse_op == MouseOperation::GraphDataAreaLeftCtrlPress) {
			if(event->modifiers() == Qt::ControlModifier) {
				int channel_ix = posToChannel(event->pos());
				if(channel_ix >= 0) {
					removeProbes(channel_ix);
					timemsec_t time = m_graph->posToTime(event->pos().x());
					createProbe(channel_ix, time);
					event->accept();
					return;
				}
			}

			/*
			QPoint pos = event->pos();
			if(event->modifiers() == Qt::NoModifier) {
				Graph *gr = graph();
				logMouseSelection() << "Set cross bar 1 pos to:" << pos.x() << pos.y();
				gr->setCrossBarPos1(pos);
				event->accept();
				update();
				return;
			}
			else if(event->modifiers() == Qt::ShiftModifier) {
				Graph *gr = graph();
				gr->setCrossBarPos2(pos);
				event->accept();
				update();
				return;
			}
			*/
		}
		else if(old_mouse_op == MouseOperation::GraphDataAreaLeftCtrlShiftPress) {
			int channel_ix = posToChannel(event->pos());
			if(channel_ix >= 0) {
				timemsec_t time = m_graph->posToTime(event->pos().x());
				createProbe(channel_ix, time);
				event->accept();
				return;
			}
		}
		else if(old_mouse_op == MouseOperation::GraphAreaSelection) {
			bool zoom_vertically = event->modifiers() == Qt::ShiftModifier;
			graph()->zoomToSelection(zoom_vertically);
			graph()->setSelectionRect(QRect());
			event->accept();
			update();
			return;
		}
		else if(old_mouse_op == MouseOperation::ChannelHeaderResize) {
			m_resizeChannelIx = -1;
			event->accept();
			update();
			return;
		}
	}
	else if(event->button() == Qt::RightButton) {
		if(old_mouse_op == MouseOperation::GraphDataAreaRightPress) {
			if(event->modifiers() == Qt::NoModifier) {
				int ch_ix = posToChannel(event->pos());
				if(ch_ix >= 0) {
					showChannelContextMenu(ch_ix, event->pos());
					event->accept();
					update();
					return;
				}
			}
		}
	}
	Super::mouseReleaseEvent(event);
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event)
{
	//logMouseSelection() << event->pos().x() << event->pos().y() << (int)m_mouseOperation;
	Super::mouseMoveEvent(event);
	QPoint pos = event->pos();
	Graph *gr = graph();
	if(isMouseAboveLeftMiniMapHandle(pos) || isMouseAboveRightMiniMapHandle(pos)) {
		//shvDebug() << (vpos.x() - gr->miniMapRect().left()) << (vpos.y() - gr->miniMapRect().top());
		setCursor(QCursor(Qt::SplitHCursor));
	}
	else if(isMouseAboveMiniMapSlider(pos)) {
		//shvDebug() << (vpos.x() - gr->miniMapRect().left()) << (vpos.y() - gr->miniMapRect().top());
		setCursor(QCursor(Qt::SizeHorCursor));
	}
	else if((isMouseAboveChannelResizeHandle(pos)) || (m_mouseOperation == MouseOperation::ChannelHeaderResize)) {
		setCursor(QCursor(Qt::SizeVerCursor));
	}
	else {
		setCursor(QCursor(Qt::ArrowCursor));
	}

	switch (m_mouseOperation) {
	case MouseOperation::None:
		break;
	case MouseOperation::MiniMapLeftResize: {
		timemsec_t t = gr->miniMapPosToTime(pos.x());
		XRange r = gr->xRangeZoom();
		r.min = t;
		if(r.interval() > 0) {
			gr->setXRangeZoom(r);
			update();
		}
		return;
	}
	case MouseOperation::MiniMapRightResize: {
		timemsec_t t = gr->miniMapPosToTime(pos.x());
		XRange r = gr->xRangeZoom();
		r.max = t;
		if(r.interval() > 0) {
			gr->setXRangeZoom(r);
			update();
		}
		return;
	}
	case MouseOperation::MiniMapScrollZoom: {
		timemsec_t t2 = gr->miniMapPosToTime(pos.x());
		timemsec_t t1 = gr->miniMapPosToTime(m_recentMousePos.x());
		m_recentMousePos = pos;
		XRange r = gr->xRangeZoom();
		shvDebug() << "r.min:" << r.min << "r.max:" << r.max;
		timemsec_t dt = t2 - t1;
		shvDebug() << dt << "r.min:" << r.min << "-->" << (r.min + dt);
		r.min += dt;
		r.max += dt;
		if(r.min >= gr->xRange().min && r.max <= gr->xRange().max) {
			gr->setXRangeZoom(r);
			r = gr->xRangeZoom();
			shvDebug() << "new r.min:" << r.min << "r.max:" << r.max;
			update();
		}
		return;
	}
	case MouseOperation::GraphDataAreaLeftPress: {
		QPoint point = pos - m_recentMousePos;
		if (point.manhattanLength() > 3) {
			m_mouseOperation = MouseOperation::GraphAreaSelection;
		}
		break;
	}
	case MouseOperation::GraphAreaSelection: {
		gr->setSelectionRect(QRect(m_recentMousePos, pos));
		update();
		break;
	}
	case MouseOperation::GraphAreaMove:
	case MouseOperation::GraphDataAreaLeftCtrlPress: {
		m_mouseOperation = MouseOperation::GraphAreaMove;
		timemsec_t t0 = gr->posToTime(m_recentMousePos.x());
		timemsec_t t1 = gr->posToTime(pos.x());
		timemsec_t dt = t0 - t1;
		XRange r = gr->xRangeZoom();
		r.min += dt;
		r.max += dt;
		gr->setXRangeZoom(r);
		m_recentMousePos = pos;
		update();
		return;
	}
	case MouseOperation::GraphDataAreaRightPress:
	case MouseOperation::GraphDataAreaLeftCtrlShiftPress:
		return;
	case MouseOperation::ChannelHeaderResize: {
		gr->resizeChannel(m_resizeChannelIx, pos.y() - m_recentMousePos.y());
		m_recentMousePos = pos;
		return;
	}
	case MouseOperation::ChannelHeaderMove:
		m_mouseOperation = MouseOperation::None;
		if (gr->channelCount() == 0) {
			return;
		}

		QRect header_rect;
		int dragged_channel = -1;
		for (int i = 0; i < gr->channelCount(); ++i) {
			const GraphChannel *ch = gr->channelAt(i);
			if (ch->verticalHeaderRect().contains(pos)) {
				header_rect = ch->verticalHeaderRect();
				dragged_channel = i;
				break;
			}
		}
		if (dragged_channel != -1) {
			QDrag *drag = new QDrag(this);
			QMimeData *mime = new QMimeData;
			mime->setText(QString());
			drag->setMimeData(mime);
			QPoint p = mapToGlobal(header_rect.topLeft());
			drag->setPixmap(screen()->grabWindow(QDesktopWidget().winId(), p.x(), p.y(), header_rect.width(), header_rect.height()));
			drag->setHotSpot(mapToGlobal(pos) - p);
			setAcceptDrops(true);

			m_channelHeaderMoveContext = new ChannelHeaderMoveContext(this);
			connect(m_channelHeaderMoveContext->mouseMoveScrollTimer, &QTimer::timeout, this, &GraphWidget::scrollToCurrentMousePosOnDrag);
			m_channelHeaderMoveContext->draggedChannel = dragged_channel;
			m_channelHeaderMoveContext->channelDropMarker->resize(header_rect.width(), QFontMetrics(font()).height() / 2);
			m_channelHeaderMoveContext->channelDropMarker->show();
			drag->exec();
			event->accept();
			setAcceptDrops(false);
			delete m_channelHeaderMoveContext;
			m_channelHeaderMoveContext = nullptr;
		}
		return;
	}

	int ch_ix = posToChannel(pos);
	if(ch_ix >= 0 && !isMouseAboveMiniMap(pos)) {
		setCursor(Qt::BlankCursor);
		gr->setCrossHairPos({ch_ix, pos});
		timemsec_t t = gr->posToTime(pos.x());
		const GraphChannel *ch = gr->channelAt(ch_ix);
		//update(ch->graphAreaRect());
		GraphModel::ChannelInfo &channel_info = gr->model()->channelInfo(ch->modelIndex());
		Sample s;
		if (channel_info.typeDescr.sampleType == shv::core::utils::ShvLogTypeDescr::SampleType::Discrete) {
			s = gr->nearestSample(ch_ix, t);
		}
		else {
			s = gr->timeToSample(ch_ix, t);
		}
		QPoint point;
		QString text;

		if (s.isValid()) {
			if (channel_info.typeDescr.sampleType == shv::core::utils::ShvLogTypeDescr::SampleType::Continuous ||
				(channel_info.typeDescr.sampleType == shv::core::utils::ShvLogTypeDescr::SampleType::Discrete && qAbs(pos.x() - gr->timeToPos(s.time)) < gr->u2px(1.1))) {
				point = mapToGlobal(pos + QPoint{gr->u2px(0.8), 0});
				QDateTime dt = QDateTime::fromMSecsSinceEpoch(s.time);
				dt = dt.toTimeZone(graph()->timeZone());

				if (channel_info.typeDescr.type == shv::core::utils::ShvLogTypeDescr::Type::Map) {
					text = QStringLiteral("%1\nx: %2\n")
						   .arg(ch->shvPath())
						   .arg(dt.toString(Qt::ISODateWithMs));
					QMap<QString, QString> value_map = m_graph->prettyMapValue(s.value, channel_info.typeDescr);
					for (auto it = value_map.begin(); it != value_map.end(); ++it) {
						text += it.key() + ": " + it.value() + "\n";
					}
					text.chop(1);
				}
				else if (channel_info.typeDescr.type == shv::core::utils::ShvLogTypeDescr::Type::BitField) {
					text = QStringLiteral("%1\nx: %2\n")
						   .arg(ch->shvPath())
						   .arg(dt.toString(Qt::ISODateWithMs));
					if (s.value.type() == QVariant::Int) {
						text += graph()->prettyBitFieldValue(s.value, channel_info.typeDescr);
					}
					else {
						text += "value: " + s.value.toString();
					}
				}
				else if (channel_info.typeDescr.type == shv::core::utils::ShvLogTypeDescr::Type::IMap) {
					text = QStringLiteral("%1\nx: %2\n")
						   .arg(ch->shvPath())
						   .arg(dt.toString(Qt::ISODateWithMs));
					QMap<QString, QString> value_map = m_graph->prettyIMapValue(s.value, channel_info.typeDescr);
					for (auto it = value_map.begin(); it != value_map.end(); ++it) {
						text += it.key() + ": " + it.value() + "\n";
					}
					text.chop(1);
				}
				else if (channel_info.typeDescr.type == shv::core::utils::ShvLogTypeDescr::Type::Enum) {
					text = QStringLiteral("%1\nx: %2\nvalue: %3")
						   .arg(ch->shvPath())
						   .arg(dt.toString(Qt::ISODateWithMs))
						   .arg(m_graph->model()->typeDescrFieldName(channel_info.typeDescr, s.value.toInt()));
				}
				else {
					text = QStringLiteral("%1\nx: %2\ny: %3\nvalue: %4")
						   .arg(ch->shvPath())
						   .arg(dt.toString(Qt::ISODateWithMs))
						   .arg(ch->posToValue(pos.y()))
						   .arg(s.value.toString());
				}
			}
		}

		QToolTip::showText(point, text);
	}
	else {
		hideCrossHair();
	}

	ch_ix = posToChannelVerticalHeader(pos);
	if(ch_ix > -1) {
		const GraphChannel *ch = gr->channelAt(ch_ix);
		QString text = tr("Channel:") + " " + ch->shvPath();
		QToolTip::showText(mapToGlobal(pos + QPoint{gr->u2px(0.2), 0}), text, this);
	}
}

void GraphWidget::moveDropMarker(const QPoint &mouse_pos)
{
	Q_ASSERT(m_channelHeaderMoveContext);
	Graph *gr = graph();
	int ix = targetChannel(mouse_pos);
	if (ix < gr->channelCount()) {
		QRect ch_rect = gr->channelAt(ix)->verticalHeaderRect();
		m_channelHeaderMoveContext->channelDropMarker->move(ch_rect.left(), ch_rect.bottom() - m_channelHeaderMoveContext->channelDropMarker->height() / 2);
	}
	else {
		QRect ch_rect = gr->channelAt(ix - 1)->verticalHeaderRect();
		m_channelHeaderMoveContext->channelDropMarker->move(ch_rect.left(), ch_rect.top() - m_channelHeaderMoveContext->channelDropMarker->height() / 2);
	}
}

int GraphWidget::targetChannel(const QPoint &mouse_pos) const
{
	const Graph *gr = graph();
	for (int i = 0; i < gr->channelCount(); ++i) {
		const GraphChannel *ch = gr->channelAt(i);
		QRect ch_rect = ch->verticalHeaderRect();
		if (ch_rect.contains(mouse_pos)) {
			if (mouse_pos.y() - ch_rect.top() > ch_rect.bottom() - mouse_pos.y()) {
				return i;
			}
			else {
				int j = i;
				while (true) {
					++j;
					if (j >= gr->channelCount()) {
						return i + 1;
					}
					if (gr->channelAt(j)->verticalHeaderRect().height()) {
						return j;
					}
				}
			}
		}
	}
	return 0;
}

void GraphWidget::scrollToCurrentMousePosOnDrag()
{
	QPoint mouse_pos = QCursor::pos();
	scrollByMouseOuterOverlap(mouse_pos);
}

bool GraphWidget::scrollByMouseOuterOverlap(const QPoint &mouse_pos)
{
	auto *view_port = parentWidget();
	auto *scroll_area = qobject_cast<GraphView*>(view_port->parentWidget());
	QRect g = view_port->geometry();
	int view_port_top = view_port->mapToGlobal(g.topLeft()).y();
	int view_port_bottom = view_port->mapToGlobal(QPoint(g.left(), g.bottom() - graph()->southFloatingBarRect().height())).y();
	auto *vs = scroll_area->verticalScrollBar();
	if (mouse_pos.y() - view_port_top < 5) {
		int diff = view_port_top - mouse_pos.y();
		if (diff < 5) {
			diff = 5;
		}
		vs->setValue(vs->value() - diff);
		moveDropMarker(mapFromGlobal(mouse_pos));
		return true;
	}
	else if (mouse_pos.y() + 5 > view_port_bottom) {
		int diff = mouse_pos.y() - view_port_bottom;
		if (diff < 5) {
			diff = 5;
		}
		vs->setValue(vs->value() + diff);
		moveDropMarker(mapFromGlobal(mouse_pos));
		return true;
	}
	return false;
}

void GraphWidget::leaveEvent(QEvent *)
{
	hideCrossHair();
}

void GraphWidget::hideCrossHair()
{
	Graph *gr = graph();
	if(gr->crossHairPos().isValid()) {
		gr->setCrossHairPos({});
		setCursor(Qt::ArrowCursor);
		QToolTip::showText(QPoint(), QString());
		update();
	}
}

void GraphWidget::wheelEvent(QWheelEvent *event)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	QPoint pos = event->pos();
#else
	QPoint pos = event->position().toPoint();
#endif
	bool is_zoom_on_slider = isMouseAboveMiniMapSlider(pos);
	bool is_zoom_on_graph = (event->modifiers() == Qt::ControlModifier) && posToChannel(pos) >= 0;
	static constexpr int ZOOM_STEP = 10;
	if(is_zoom_on_slider) {
		Graph *gr = graph();
		double deg = event->angleDelta().y();
		//deg /= 120;
		// 120 deg ~ 1/20 of range
		timemsec_t dt = static_cast<timemsec_t>(deg * gr->xRangeZoom().interval() / 120 / ZOOM_STEP);
		XRange r = gr->xRangeZoom();
		r.min += dt;
		r.max -= dt;
		if(r.interval() > 1) {
			gr->setXRangeZoom(r);
			update();
		}
		event->accept();
		return;
	}
	if(is_zoom_on_graph) {
		Graph *gr = graph();
		double deg = event->angleDelta().y();
		//deg /= 120;
		// 120 deg ~ 1/20 of range
		timemsec_t dt = static_cast<timemsec_t>(deg * gr->xRangeZoom().interval() / 120 / ZOOM_STEP);
		XRange r = gr->xRangeZoom();
		double ratio = 0.5;
		// shift new zoom to center it horizontally on the mouse position
		timemsec_t t_mouse = gr->posToTime(pos.x());
		ratio = static_cast<double>(t_mouse - r.min) / r.interval();
		r.min += ratio * dt;
		r.max -= (1 - ratio) * dt;
		if(r.interval() > 1) {
			gr->setXRangeZoom(r);
			//r = gr->xRangeZoom();
			update();
		}
		event->accept();
		return;
	}

	Super::wheelEvent(event);
}

void GraphWidget::contextMenuEvent(QContextMenuEvent *event)
{
	shvLogFuncFrame();
	if(!m_graph)
		return;
	const QPoint pos = event->pos();
	if(m_graph->cornerCellRect().contains(pos)) {
		showGraphContextMenu(pos);
		return;
	}
	for (int i = 0; i < m_graph->channelCount(); ++i) {
		const GraphChannel *ch = m_graph->channelAt(i);
		if(ch->verticalHeaderRect().contains(pos)) {
			showChannelContextMenu(i, pos);
		}
	}
}

void GraphWidget::dragEnterEvent(QDragEnterEvent *event)
{
	event->accept();
}

void GraphWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
	Q_UNUSED(event);
}

void GraphWidget::dragMoveEvent(QDragMoveEvent *event)
{
	Q_ASSERT(m_channelHeaderMoveContext);
	QPoint pos = event->pos();

	if (scrollByMouseOuterOverlap(mapToGlobal(pos))) {
		m_channelHeaderMoveContext->mouseMoveScrollTimer->start();
	}
	else {
		m_channelHeaderMoveContext->mouseMoveScrollTimer->stop();
	}

	moveDropMarker(pos);
}

void GraphWidget::dropEvent(QDropEvent *event)
{
	Q_ASSERT(m_channelHeaderMoveContext);
	int target_channel = targetChannel(event->pos());
	if (target_channel != m_channelHeaderMoveContext->draggedChannel) {
		graph()->moveChannel(m_channelHeaderMoveContext->draggedChannel, target_channel);
	}
	event->accept();
}

void GraphWidget::showGraphContextMenu(const QPoint &mouse_pos)
{
	QMenu menu(this);
	menu.addAction(tr("Show all channels"), [this]() {
		m_graph->showAllChannels();
	});
	menu.addAction(tr("Hide channels without changes"), [this]() {
		m_graph->hideFlatChannels();
	});
	menu.addAction(tr("Reset channel headers"), [this]() {
		m_graph->createChannelsFromModel();
		auto graph_filter = m_graph->channelFilter();
		graph_filter.setMatchingPaths(m_graph->channelPaths());
		m_graph->setChannelFilter(graph_filter);
	});
	if (m_graph->isYAxisVisible()) {
		menu.addAction(tr("Hide Y axis"), [this]() {
			m_graph->setYAxisVisible(false);
		});
	}
	else {
		menu.addAction(tr("Show Y axis"), [this]() {
			m_graph->setYAxisVisible(true);
		});
	}

	if(menu.actions().count())
		menu.exec(mapToGlobal(mouse_pos));
}

void GraphWidget::showChannelContextMenu(int channel_ix, const QPoint &mouse_pos)
{
	shvLogFuncFrame();
	const GraphChannel *ch = m_graph->channelAt(channel_ix, !shv::core::Exception::Throw);
	if(!ch)
		return;
	QMenu menu(this);
	if(ch->isMaximized()) {
		menu.addAction(tr("Normal size"), [this, channel_ix]() {
			m_graph->setChannelMaximized(channel_ix, false);
		});
	}
	else {
		menu.addAction(tr("Maximize"), [this, channel_ix]() {
			m_graph->setChannelMaximized(channel_ix, true);
		});
	}
	menu.addAction(tr("Hide"), [this, channel_ix]() {
		m_graph->setChannelVisible(channel_ix, false);
	});
	menu.addAction(tr("Reset X-zoom"), [this]() {
		//shvInfo() << "settings";
		timeline::GraphModel *m = m_graph->model();
		if(!m)
			return;
		timeline::XRange rng = m->xRange();
		m_graph->setXRange(rng);
		this->update();
	});
	menu.addAction(tr("Reset Y-zoom"), [this, channel_ix, ch]() {
		//shvInfo() << "settings";
		timeline::GraphModel *m = m_graph->model();
		if(!m)
			return;
		int chix = ch->modelIndex();
		timeline::YRange rng = m->yRange(chix);
		m_graph->setYRange(channel_ix, rng);
		this->update();
	});
	menu.addAction(tr("Set probe (Ctrl + Left mouse)"), [this, channel_ix, mouse_pos]() {
		removeProbes(channel_ix);
		timemsec_t time = m_graph->posToTime(mouse_pos.x());
		createProbe(channel_ix, time);
	});
	menu.addAction(tr("Add probe (Ctrl + Shift + Left mouse)"), [this, channel_ix, mouse_pos]() {
			timemsec_t time = m_graph->posToTime(mouse_pos.x());
			createProbe(channel_ix, time);
		});
	if(menu.actions().count())
		menu.exec(mapToGlobal(mouse_pos));
}

void GraphWidget::createProbe(int channel_ix, timemsec_t time)
{
	const GraphChannel *ch = m_graph->channelAt(channel_ix);
	GraphModel::ChannelInfo &channel_info = m_graph->model()->channelInfo(ch->modelIndex());

	if (channel_info.typeDescr.sampleType == shv::core::utils::ShvLogTypeDescr::SampleType::Discrete) {
		Sample s = m_graph->nearestSample(channel_ix, time);

		if (s.isValid())
			time = s.time;
	}

	ChannelProbe *probe = m_graph->addChannelProbe(channel_ix, time);
	Q_ASSERT(probe);

	connect(probe, &ChannelProbe::currentTimeChanged, probe, [this]() {
		update();
	});

	ChannelProbeWidget *w = new ChannelProbeWidget(probe, this);

	connect(w, &ChannelProbeWidget::destroyed, this, [this, probe]() {
		m_graph->removeChannelProbe(probe);
		update();
	});

	w->show();
	QPoint pos(m_graph->timeToPos(time) - (w->width() / 2), -geometry().top() - w->height() - m_graph->u2px(0.2));
	w->move(mapToGlobal(pos));
	update();
}

void GraphWidget::removeProbes(int channel_ix)
{
	QList<ChannelProbeWidget*> probe_widgets = findChildren<ChannelProbeWidget*>(QString(), Qt::FindDirectChildrenOnly);

	for (ChannelProbeWidget *pw: probe_widgets) {
		if (pw->probe()->channelIndex() == channel_ix)
			pw->close();
	}
}

bool GraphWidget::isMouseAboveChannelResizeHandle(const QPoint &mouse_pos) const
{
	const Graph *gr = graph();
	const int MARGIN = gr->u2px(0.5);

	int channel_ix = gr->posToChannelHeader(mouse_pos);

	if (channel_ix > -1) {
		const GraphChannel *ch = gr->channelAt(channel_ix);
		QRect header_rect = ch->verticalHeaderRect();
		return (header_rect.bottom() - mouse_pos.y() < MARGIN);
	}

	return false;
}

GraphWidget::ChannelHeaderMoveContext::ChannelHeaderMoveContext(QWidget *parent)
{
	mouseMoveScrollTimer = new QTimer(parent);
	mouseMoveScrollTimer->setInterval(100);
	channelDropMarker = new QWidget(parent);
	QPalette pal = channelDropMarker->palette();
	pal.setColor(QPalette::ColorRole::Window, Qt::yellow);
	pal.setColor(QPalette::ColorRole::Base, Qt::yellow);
	pal.setColor(QPalette::ColorRole::ToolTipBase, Qt::yellow);
	channelDropMarker->setAutoFillBackground(true);
	channelDropMarker->setPalette(pal);
	channelDropMarker->hide();
}

GraphWidget::ChannelHeaderMoveContext::~ChannelHeaderMoveContext()
{
	delete mouseMoveScrollTimer;
	delete channelDropMarker;
}

}}}

