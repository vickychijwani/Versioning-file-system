#include <iostream>
#include <cstdlib>
#include <cmath>
#include <unistd.h>

#define FILE_EXISTS(x) (access( QString(x).toLatin1().data() , F_OK ) == 0)

#include "window.h"
#include "filechooser.h"
#include "params.h"

GraphWindow::GraphWindow(char *filepath) {
	this->filepath = filepath;
	QString *filepathString = new QString(filepath);
	QStringList filePieces = filepathString->split("/");
	int size = filePieces.size();
	this->filename = filePieces.value(size-1);
	mountdir_path = "";
	foreach(QString piece, filePieces) {
		mountdir_path += piece+"/";
		if(piece.compare("mountdir")==0)
			break;
	}
	
	loadFileData(this->filepath);
	
	this->current = NULL;
	
	scene = new QGraphicsScene(0, 0, TIMELINE_DEFAULT_WIDTH, TIMELINE_DEFAULT_HEIGHT);
	
	setupModel();
	setupViews();
	
	QScrollBar *horizontalScrollBar = view->horizontalScrollBar();
	horizontalScrollBar->setValue(horizontalScrollBar->maximum());
	
	QList<int> sizes;
	sizes.append(scene->height());
	sizes.append(EDITOR_DEFAULT_HEIGHT);
	sizes.append(EDITOR_DEFAULT_HEIGHT);
	
	splitter->setSizes(sizes);
	
	connect(scene, SIGNAL(selectionChanged()), this, SLOT(showDocument()));
	
	if(MAXIMIZED)
		showMaximized();
	setWindowTitle("Timeline");
	setMaximumSize(QApplication::desktop()->screenGeometry().width(), QApplication::desktop()->screenGeometry().height());
	
	FileChooser *fileChooser = new FileChooser(filepath);
	
	QTabWidget *tabWidget = new QTabWidget;
	tabWidget->addTab(this, "Timeline");
	tabWidget->addTab(fileChooser, "Space Analysis");
	tabWidget->showMaximized();
	tabWidget->show();
}

Point *GraphWindow::getRoot() const { return root; }
Point *GraphWindow::getCurrent() const { return current; }
QList<Point *> *GraphWindow::getPoints() const { return points; }
QGraphicsScene *GraphWindow::getScene() const { return scene; }
QString GraphWindow::getMountDirPath() const { return mountdir_path; }

void GraphWindow::setRoot(Point *p) { root = p; }
void GraphWindow::setCurrent(Point *current) {
	if(this->current != NULL) {
		this->current->setCurrent(false);
		QStyleOptionGraphicsItem *option = new QStyleOptionGraphicsItem;
		option->state = QStyle::State_None;
		this->current->paint(new QPainter, option, NULL);
	}
	this->current = current;
	current->setCurrent(true);
	QStyleOptionGraphicsItem *option = new QStyleOptionGraphicsItem;
	option->state = QStyle::State_None;
	this->current->paint(new QPainter, option, NULL);
}

void GraphWindow::loadFileData(QString filepath) {
	QString command = "cd "+mountdir_path+" && __guidata "+filepath;
	int status = system(command.toLatin1().data());
	if(status == 0)
		sleep(1);
}

void GraphWindow::setAncestorCount() {
	Point *p, *p1, *p2;
	for(int i=0; i<points->size(); i++) {
		p = points->at(i);
		p1 = p;
		p2 = p->getParent();
		while(p2 != 0) {
			p1 = p2;
			p2 = p1->getParent();
			p->incrementAncestorCount();
		}
	}
}

void GraphWindow::setupViews() {
	// timeline view
	view = new QGraphicsView;
	view->setScene(scene);
	view->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
	view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	view->setMinimumSize(0, scene->height()+20);
	view->setBaseSize(scene->width(), scene->height()+20);
	
	QPointF *p1 = new QPointF(LEFT_MARGIN, TOP_MARGIN);
	QPointF *p2 = new QPointF(maxX, TOP_MARGIN);
	axis = new Axis(p1, p2);
	axis->drawTicks(scene, rootX, scalingFactor);
	scene->addItem(axis);
	
	// widget for timeline + toolbarLayout
	QVBoxLayout *topLayout = new QVBoxLayout;
	topLayout->addWidget(view);
	
	QWidget *topWidget = new QWidget(this);
	topWidget->setLayout(topLayout);
	
	// single editor for when a single point is clicked
	singleEditorLabel = new QLabel("Document");
	
	singleEditor = new QTextEdit(this);
	singleEditor->setReadOnly(true);
	singleEditor->setBaseSize(EDITOR_DEFAULT_WIDTH, EDITOR_DEFAULT_HEIGHT);
	
	QVBoxLayout *singleEditorLayout = new QVBoxLayout;
	singleEditorLayout->addWidget(singleEditorLabel);
	singleEditorLayout->addWidget(singleEditor);
	
	singleEditorWidget = new QWidget(this);
	singleEditorWidget->setLayout(singleEditorLayout);
	singleEditorWidget->hide();
	
	// double editor for when 2 points are clicked
	// Editor #1
	doubleEditorLeftLabel = new QLabel("Document");
	
	doubleEditorLeft = new QTextEdit(this);
	doubleEditorLeft->setReadOnly(true);
	doubleEditorLeft->setBaseSize(EDITOR_DEFAULT_WIDTH, EDITOR_DEFAULT_HEIGHT);
	doubleEditorLeft->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	doubleEditorLeft->setContentsMargins(0,0,0,0);
	doubleEditorLeft->setWordWrapMode(QTextOption::NoWrap);
	
	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(doubleEditorLeft);
	
	QVBoxLayout *doubleEditorLeftLayout = new QVBoxLayout;
	doubleEditorLeftLayout->addWidget(doubleEditorLeftLabel);
	doubleEditorLeftLayout->addLayout(layout);
	
	// Editor #2
	doubleEditorRightLabel = new QLabel("Document");
	
	doubleEditorRight = new QTextEdit(this);
	doubleEditorRight->setReadOnly(true);
	doubleEditorRight->setBaseSize(EDITOR_DEFAULT_WIDTH, EDITOR_DEFAULT_HEIGHT);
	doubleEditorRight->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	doubleEditorRight->setContentsMargins(0,0,0,0);
	doubleEditorRight->setWordWrapMode(QTextOption::NoWrap);
	
	QVBoxLayout *doubleEditorRightLayout = new QVBoxLayout;
	doubleEditorRightLayout->addWidget(doubleEditorRightLabel);
	doubleEditorRightLayout->addWidget(doubleEditorRight);
	
	// Create and connect common vertical scroll bar to editors, and connect horizontal scroll bars
	QScrollBar *leftVerticalScrollBar = doubleEditorLeft->verticalScrollBar();
	QScrollBar *rightVerticalScrollBar = doubleEditorRight->verticalScrollBar();
	QScrollBar *leftHorizontalScrollBar = doubleEditorLeft->horizontalScrollBar();
	QScrollBar *rightHorizontalScrollBar = doubleEditorRight->horizontalScrollBar();
	doubleEditorVerticalScrollBar = new QScrollBar(Qt::Vertical, this);
	
	connect( leftVerticalScrollBar , SIGNAL(valueChanged(int)) , rightVerticalScrollBar , SLOT(setValue(int)) );
	connect( rightVerticalScrollBar , SIGNAL(valueChanged(int)) , leftVerticalScrollBar , SLOT(setValue(int)) );
	connect( doubleEditorVerticalScrollBar , SIGNAL(valueChanged(int)) , leftVerticalScrollBar , SLOT(setValue(int)) );
	connect( doubleEditorVerticalScrollBar , SIGNAL(valueChanged(int)) , rightVerticalScrollBar , SLOT(setValue(int)) );
	connect( leftVerticalScrollBar , SIGNAL(valueChanged(int)) , doubleEditorVerticalScrollBar , SLOT(setValue(int)) );
	connect( rightVerticalScrollBar , SIGNAL(valueChanged(int)) , doubleEditorVerticalScrollBar , SLOT(setValue(int)) );
	
	connect( leftHorizontalScrollBar , SIGNAL(valueChanged(int)) , rightHorizontalScrollBar , SLOT(setValue(int)) );
	connect( rightHorizontalScrollBar , SIGNAL(valueChanged(int)) , leftHorizontalScrollBar , SLOT(setValue(int)) );
	
	connect( leftVerticalScrollBar , SIGNAL(rangeChanged(int,int)) , this , SLOT(updateRange(int,int)) ) ;
	connect( rightVerticalScrollBar , SIGNAL(rangeChanged(int,int)) , this , SLOT(updateRange(int,int)) ) ;
	
	layout->addWidget(doubleEditorVerticalScrollBar);
	
	// Double Editor Widget
	QHBoxLayout *doubleEditorLayout = new QHBoxLayout;
	doubleEditorLayout->addLayout(doubleEditorLeftLayout);
	doubleEditorLayout->addLayout(doubleEditorRightLayout);
	
	doubleEditorWidget = new QWidget;
	doubleEditorWidget->setLayout(doubleEditorLayout);
	doubleEditorWidget->hide();
	
	splitter = new QSplitter(Qt::Vertical);
	splitter->addWidget(topWidget);
	splitter->addWidget(singleEditorWidget);
	splitter->addWidget(doubleEditorWidget);
	
	setCentralWidget(splitter);
}

void GraphWindow::setupModel() {
	QString TEMP_PREFIX = "/tmp/rvfs/";
	readFromFile(TEMP_PREFIX+filename);
	setAncestorCount();
	
	for(int i=0; i<points->size(); i++) {
		Point *p = points->at(i);
		
		scene->addItem(p);
		
		if(p->childCount() == 0) {
			p->setOutlineColor(POINT_HEAD_OUTLINE_COLOR);
			p->setOutlineWidth(POINT_HEAD_OUTLINE_WIDTH);
		}
		
		if(p->getParent() != 0) {
			Line *line = new Line(p->getParent(), p, 5.0);
			scene->addItem(line);
			
			p->setParentItem(p->getParent());
			p->installSceneEventFilter(p->getParent());
		}
	}
}

void GraphWindow::showDocument() {
	QString TEMP_PREFIX = "/tmp/rvfs/";
	QString TEMP_FILE_NAME_1 = "switch1";
	QString TEMP_FILE_NAME_2 = "switch2";
	QString DIFF_FILE_NAME = "diff";
	QString command;
	
	QFile *file1 = new QFile(TEMP_PREFIX+TEMP_FILE_NAME_1);
	QFile *file2 = new QFile(TEMP_PREFIX+TEMP_FILE_NAME_2);
	
	command = "rm "+ TEMP_PREFIX+TEMP_FILE_NAME_1 +" "+ TEMP_PREFIX+TEMP_FILE_NAME_2;
	int status = system(command.toLatin1().data());
	
	if(status != 0)
		return;
	
	QList<QGraphicsItem *> selectedPoints = scene->selectedItems();
	
	QFile diff(TEMP_PREFIX+DIFF_FILE_NAME);
	if(diff.exists())
		diff.remove();
	
	// 1 point selected
	if(selectedPoints.size() == 1) {
		singleEditor->setText("");
		doubleEditorLeft->setText("");
		doubleEditorRight->setText("");
		doubleEditorWidget->hide();
		singleEditorWidget->show();
		
		Point *p = dynamic_cast<Point *>(selectedPoints.at(0));
		
		QString versionTime = QDateTime::fromMSecsSinceEpoch(1000*( qint64(p->data(POINT_TIMESTAMP_INDEX).toUInt()) )).toString("hh:mm:ss AP, d MMM, yyyy");
		singleEditorLabel->setText(filename+" (created on "+versionTime+")");
		
		command = "cd "+mountdir_path+" && __guiswitch "+p->getRelativeFilePath()+" "+p->data(POINT_OFFSET_INDEX).toString()+" "+p->data(POINT_LOPO_INDEX).toString();
		int status = system(command.toLatin1().data());
		
		QFile logFile("/tmp/log1");
		if(logFile.open(QFile::Append | QFile::Text)) {
			QTextStream logStream(&logFile);
			logStream << command << "\t" << status << "\n";
			logFile.close();
		}
		
		sleep(0.5);
		
		if(file1->open(QFile::ReadOnly | QFile::Text)) {
			QTextStream stream(file1);
			singleEditor->setText(stream.readAll());
		}
	}
	
	// 2 points selected
	else if(selectedPoints.size() == 2) {
		singleEditor->setText("");
		doubleEditorLeft->setText("");
		doubleEditorRight->setText("");
		singleEditorWidget->hide();
		doubleEditorWidget->show();
		
		// Point #1
		Point *p1 = dynamic_cast<Point *>(selectedPoints.at(0));
		
		command = "cd "+mountdir_path+" && __guiswitch "+p1->getRelativeFilePath()+" "+p1->data(POINT_OFFSET_INDEX).toString()+" "+p1->data(POINT_LOPO_INDEX).toString();
		int status1 = system(command.toLatin1().data());
		
		sleep(0.5);
		
		// Point #2
		Point *p2 = dynamic_cast<Point *>(selectedPoints.at(1));
		
		command = "cd "+mountdir_path+" && __guiswitch "+p2->getRelativeFilePath()+" "+p2->data(POINT_OFFSET_INDEX).toString()+" "+p2->data(POINT_LOPO_INDEX).toString();
		int status2 = system(command.toLatin1().data());
		
		if(status1 != 0 || status2 != 0)
			return;
		
		view->setInteractive(true);
		sleep(0.5);
		
		QString versionTime1 = QDateTime::fromMSecsSinceEpoch(1000*( qint64(p1->data(POINT_TIMESTAMP_INDEX).toUInt()) )).toString("hh:mm:ss AP, d MMM, yyyy");
		doubleEditorLeftLabel->setText(filename+" (created on "+versionTime1+")");
		QString versionTime2 = QDateTime::fromMSecsSinceEpoch(1000*( qint64(p2->data(POINT_TIMESTAMP_INDEX).toUInt()) )).toString("hh:mm:ss AP, d MMM, yyyy");
		doubleEditorRightLabel->setText(filename+" (created on "+versionTime2+")");
		
		if(file1->open(QFile::ReadOnly | QFile::Text)) {
			QTextStream stream1(file1);
			doubleEditorLeft->setText(stream1.readAll());
		}
		
		if(file2->open(QFile::ReadOnly | QFile::Text)) {
			QTextStream stream2(file2);
			doubleEditorRight->setText(stream2.readAll());
		}
		
		highlightDifferences();
		
		file1->close();
		file2->close();
	}
	// other selections
	else {
		scene->clearSelection();
		singleEditor->setText("");
		doubleEditorLeft->setText("");
		doubleEditorRight->setText("");
		singleEditorWidget->hide();
		doubleEditorWidget->hide();
	}
}

void GraphWindow::highlightDifferences() {
	QString TEMP_PREFIX = "/tmp/rvfs/";
	QString TEMP_FILE_NAME_1 = "switch1";
	QString TEMP_FILE_NAME_2 = "switch2";
	QString DIFF_FILE_NAME = "diff";
	
	QList<QTextEdit::ExtraSelection> extraSelections1, extraSelections2;
	
	QString command = "diff "+ TEMP_PREFIX+TEMP_FILE_NAME_1 +" "+ TEMP_PREFIX+TEMP_FILE_NAME_2 +">"+TEMP_PREFIX+DIFF_FILE_NAME;
	int status = system(command.toLatin1().data());
	
	if(status != -1) {
		QFile *diff_file = new QFile(TEMP_PREFIX+DIFF_FILE_NAME);
		QString thunkHead, line;
		if(diff_file->open(QFile::ReadOnly | QFile::Text)) {
			QTextStream stream(diff_file);
		
			thunkHead = stream.readLine();
		
			while(!thunkHead.isNull()) {
				if(thunkHead.contains('c')) {
					line = stream.readLine();
				
					QStringList changePieces = thunkHead.split("c", QString::SkipEmptyParts);
					QString file1Changes = changePieces.value(0);
					QString file2Changes = changePieces.value(1);
				
					if(!file1Changes.contains(",")) {
						QTextEdit::ExtraSelection selection1;
						selection1.cursor = doubleEditorLeft->textCursor();
						selection1.format.setBackground(EDITOR_COMMON_HIGHLIGHT_COLOR);
					
						int file1LineChanged = file1Changes.toInt();
					
						selection1.cursor.movePosition(QTextCursor::Start);
						selection1.cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, file1LineChanged-1);
						selection1.format.setProperty(QTextFormat::FullWidthSelection, true);
						selection1.cursor.clearSelection();
						extraSelections1.append(selection1);
					}
					else {
						QStringList file1LinesChanged = file1Changes.split(",");
						int file1LinesChangedFrom = file1LinesChanged.value(0).toInt();
						int file1LinesChangedTo = file1LinesChanged.value(1).toInt();
					
						for(int i=(file1LinesChangedFrom-1) ; i<=(file1LinesChangedTo-1) ; i++) {
							QTextEdit::ExtraSelection selection1;
							selection1.cursor = doubleEditorLeft->textCursor();
							selection1.format.setBackground(EDITOR_COMMON_HIGHLIGHT_COLOR);
						
							selection1.cursor.movePosition(QTextCursor::Start);
							selection1.cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, i);
							selection1.format.setProperty(QTextFormat::FullWidthSelection, true);
							selection1.cursor.clearSelection();
							extraSelections1.append(selection1);
						}
					}
				
					if(!file2Changes.contains(",")) {
						QTextEdit::ExtraSelection selection2;
						selection2.cursor = doubleEditorRight->textCursor();
						selection2.format.setBackground(EDITOR_COMMON_HIGHLIGHT_COLOR);
					
						int file2LineChanged = file2Changes.toInt();
					
						selection2.cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
						selection2.cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, file2LineChanged-1);
						selection2.format.setProperty(QTextFormat::FullWidthSelection, true);
						selection2.cursor.clearSelection();
						extraSelections2.append(selection2);
					}
					else {
						QStringList file2LinesChanged = file2Changes.split(",");
						int file2LinesChangedFrom = file2LinesChanged.value(0).toInt();
						int file2LinesChangedTo = file2LinesChanged.value(1).toInt();
					
						for(int i=(file2LinesChangedFrom-1) ; i<=(file2LinesChangedTo-1) ; i++) {
							QTextEdit::ExtraSelection selection2;
							selection2.cursor = doubleEditorRight->textCursor();
							selection2.format.setBackground(EDITOR_COMMON_HIGHLIGHT_COLOR);
						
							selection2.cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
							selection2.cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, i);
							selection2.format.setProperty(QTextFormat::FullWidthSelection, true);
							selection2.cursor.clearSelection();
							extraSelections2.append(selection2);
						}
					}
				
					while(line.startsWith("<") || line.startsWith(">") || line.startsWith("-")) {
						line = stream.readLine();
					}
				}
				else if(thunkHead.contains('a')) {
					line = stream.readLine();
				
					QStringList changePieces = thunkHead.split("a", QString::SkipEmptyParts);
				
					QString file1Changes = changePieces.value(0);
					QString file2Changes = changePieces.value(1);
				
					if(!file2Changes.contains(",")) {
						QTextEdit::ExtraSelection selection2;
						selection2.cursor = doubleEditorRight->textCursor();
						selection2.format.setBackground(EDITOR_EXCLUSIVE_HIGHLIGHT_COLOR);
					
						int file2LineChanged = file2Changes.toInt();
						
						selection2.cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
						selection2.cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, file2LineChanged-1);
						selection2.format.setProperty(QTextFormat::FullWidthSelection, true);
						selection2.cursor.clearSelection();
						extraSelections2.append(selection2);
					}
					else {
						QStringList file2LinesChanged = file2Changes.split(",");
						int file2LinesChangedFrom = file2LinesChanged.value(0).toInt();
						int file2LinesChangedTo = file2LinesChanged.value(1).toInt();
					
						for(int i=(file2LinesChangedFrom-1) ; i<=(file2LinesChangedTo-1) ; i++) {
							QTextEdit::ExtraSelection *selection2 = new QTextEdit::ExtraSelection;
							selection2->cursor = doubleEditorRight->textCursor();
							selection2->format.setBackground(EDITOR_EXCLUSIVE_HIGHLIGHT_COLOR);
						
							selection2->cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
							selection2->cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, i);
							selection2->format.setProperty(QTextFormat::FullWidthSelection, true);
							selection2->cursor.clearSelection();
							extraSelections2.append(*selection2);
						}
					}
				
					while(line.startsWith("<") || line.startsWith(">") || line.startsWith("-")) {
						line = stream.readLine();
					}
				}
				else if(thunkHead.contains('d')) {
					line = stream.readLine();
				
					QStringList changePieces = thunkHead.split("d", QString::SkipEmptyParts);
				
					QString file1Changes = changePieces.value(0);
					QString file2Changes = changePieces.value(1);
					sleep(0.5);
				
					if(!file1Changes.contains(",")) {
						QTextEdit::ExtraSelection selection1;
						selection1.cursor = doubleEditorLeft->textCursor();
						selection1.format.setBackground(EDITOR_EXCLUSIVE_HIGHLIGHT_COLOR);
					
						int file1LineChanged = file1Changes.toInt();
					
						selection1.cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
						selection1.cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, file1LineChanged-1);
						selection1.format.setProperty(QTextFormat::FullWidthSelection, true);
						selection1.cursor.clearSelection();
						extraSelections1.append(selection1);
					}
					else {
						QStringList file1LinesChanged = file1Changes.split(",");
						int file1LinesChangedFrom = file1LinesChanged.value(0).toInt();
						int file1LinesChangedTo = file1LinesChanged.value(1).toInt();
						
						for(int i=(file1LinesChangedFrom-1) ; i<=(file1LinesChangedTo-1) ; i++) {
							QTextEdit::ExtraSelection *selection1 = new QTextEdit::ExtraSelection;
							selection1->cursor = doubleEditorLeft->textCursor();
							selection1->format.setBackground(EDITOR_EXCLUSIVE_HIGHLIGHT_COLOR);
						
							selection1->cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
							selection1->cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, i);
							selection1->format.setProperty(QTextFormat::FullWidthSelection, true);
							selection1->cursor.clearSelection();
							extraSelections1.append(*selection1);
						}
					}
				
					while(line.startsWith("<") || line.startsWith(">") || line.startsWith("-")) {
						line = stream.readLine();
					}
				}
				thunkHead = line;
			}
		}
		diff_file->close();
	}
	
	doubleEditorLeft->setExtraSelections(extraSelections1);
	doubleEditorRight->setExtraSelections(extraSelections2);
}

void GraphWindow::updateRange(int, int) {
	QScrollBar *leftScrollBar = doubleEditorLeft->verticalScrollBar();
	QScrollBar *rightScrollBar = doubleEditorRight->verticalScrollBar();
	int leftScrollBarRange = leftScrollBar->maximum() - leftScrollBar->minimum();
	int rightScrollBarRange = rightScrollBar->maximum() - rightScrollBar->minimum();
	
	int scrollBarRangeMin, scrollBarRangeMax;
	if(leftScrollBarRange > rightScrollBarRange) {
		scrollBarRangeMax = leftScrollBar->maximum();
		scrollBarRangeMin = leftScrollBar->minimum();
	}
	else {
		scrollBarRangeMax = rightScrollBar->maximum();
		scrollBarRangeMin = rightScrollBar->minimum();
	}
	doubleEditorVerticalScrollBar->setRange( scrollBarRangeMin , scrollBarRangeMax );
}

void GraphWindow::readFromFile(const QString &path) {
	// storage format: <valid> <timestamp> <lo=0/po> <hash> <tag> <diff_lc> <parent_offset>
	points = new QList<Point *>;
	maxX = 0;
	int branchCount = 1;
	const float scalingFactors[] = {
		100.0/(10),
	};
	
	if(!path.isNull()) {
		QString treePath = path;
		QString headPath = path;
		QFile treeFile( treePath.append(".tree") );
		QFile headFile( headPath.append(".head") );
		
		if( treeFile.open(QFile::ReadOnly | QFile::Text) && headFile.open(QFile::ReadOnly | QFile::Text) ) {
			QTextStream treeStream(&treeFile);
			QTextStream headStream(&headFile);
			QString line;
			
			QString currentString = headStream.readLine();
			int currentIndex = currentString.split(" ").value(0).right(10).toInt();
			
			int counter = 0;
			int invalidCount = 0;
			
			do {
				line = treeStream.readLine();
				
				if(!line.isEmpty()) {
					QStringList pieces = line.split(" ", QString::SkipEmptyParts);
					
					bool valid = (pieces.value(0).toInt())?(true):(false);
					qreal x = pieces.value(1).toDouble();
					int timestamp = pieces.value(1).toInt();
					qreal y;
					qreal radius = POINT_DEFAULT_RADIUS;
					QString parentString = pieces.value(6);
					int parentOffset = parentString.toInt();
					int parentIndex = parentOffset/326;
					QString tagText = (pieces.value(4)=="_")?(""):(pieces.value(4));
					QString tooltipText = tagText;
					int lopo = pieces.value(2).toInt();
					
					for(int i=0; i<points->size(); i++) {
						if(points->at(i)->data(POINT_OFFSET_INDEX) == parentOffset) {
							parentIndex -= points->at(i)->getInvalidCount();
						}
					}
					
					if(!valid) {
						invalidCount++;
						counter++;
						continue;
					}
					
					if(parentString.compare("-1") == 0) {
						rootX = x;
						x = 0;
						y = TOP_MARGIN + BOTTOM_MARGIN + AXIS_BOTTOM_MARGIN + radius;
						
						// resize the height of the scene as required
						if(y > (scene->height() - BOTTOM_MARGIN))
							scene->setSceneRect(0, 0, scene->width(), y+BOTTOM_MARGIN);
						
						Point *p = new Point( filepath, scene, LEFT_MARGIN+x, y, radius, tagText, tooltipText, NULL, this );
						points->append(p);
						setRoot(p);
						
						if(LEFT_MARGIN+x > maxX)
							maxX = LEFT_MARGIN+x;
						
						p->setData(POINT_OFFSET_INDEX, 326*counter);
						p->setInvalidCount(invalidCount);
						p->setValidity(valid);
						
						if(timestamp == currentIndex) {
							this->setCurrent(p);
						}
						
						if(lopo == LO) {
							p->setData(POINT_LOPO_INDEX, 0);
						}
						p->setData(POINT_TIMESTAMP_INDEX, timestamp);
					}
					else {
						x -= rootX;
						
						Point *parent = points->at(parentIndex);
						Point *child = new Point( filepath, scene, LEFT_MARGIN+x, parent->getY(), radius, tagText, tooltipText, parent, this );
						parent->addChild(child);
						
						if(LEFT_MARGIN+x > maxX) {
							maxX = LEFT_MARGIN+x;
						}
						
						if(parent->childCount() > 1) {
							branchCount++;
							y = root->getY() + BRANCH_SEPARATION * (branchCount-1);
						}
						else {
							y = parent->getY();
						}
						
						// resize the height of the scene as required
						if(y > (scene->height() - BOTTOM_MARGIN))
							scene->setSceneRect(0, 0, scene->width(), y+BOTTOM_MARGIN);
						
						child->setY(y);
						child->updateTagPosition();
						
						points->append(child);
						
						child->setData(POINT_OFFSET_INDEX, 326*counter);
						child->setInvalidCount(invalidCount);
						child->setValidity(valid);
						
						if(timestamp == currentIndex) {
							this->setCurrent(child);
						}
						
						if(lopo == LO) {
							Point *q = child;
							do {
								q->setData(POINT_LOPO_INDEX, child->data(POINT_OFFSET_INDEX).toInt());
								q = q->getParent();
							} while( q!=NULL && q->data(POINT_LOPO_INDEX).toString().isEmpty() );
						}

						child->setData(POINT_TIMESTAMP_INDEX, timestamp);
					}
				}
				
				counter++;
			} while(!line.isEmpty());
			
			bool collisionExists;
			
			// Calculate optimum scaling factor
			for(unsigned int i=0; i<(sizeof(scalingFactors)/sizeof(float)); i++) {
				scalingFactor = scalingFactors[i];
				collisionExists = false;
				
				for(int j=0; j<points->size(); j++) {
					Point *p = points->at(j);
					if(p!=root) {
						Point *q = points->at(j)->getParent();
						if( (p->getXAtScale(scalingFactor) - q->getXAtScale(scalingFactor)) < (p->getRadius()+q->getRadius()+2*POINT_HEAD_OUTLINE_WIDTH+POINT_MINIMUM_DISTANCE) ) {
							collisionExists = true;
							break;
						}
					}
				}
				
				if(collisionExists)
					continue;
				else
					break;
			}
			
			// apply optimum scaling factor
			for(int i=0; i<points->size(); i++) {
				Point *p = points->at(i);
				qreal unscaledX = p->getX() - LEFT_MARGIN;
				p->setX(unscaledX*scalingFactor + LEFT_MARGIN);
				p->updateTagPosition();
			}
			rootX *= scalingFactor;
			maxX = (maxX - LEFT_MARGIN)*scalingFactor + LEFT_MARGIN;
			
			// set the size of the scene based on the right end of the axis
			qreal length = maxX - LEFT_MARGIN;
			int segmentCount = ((int)(length/AXIS_DEFAULT_TICK_SEPARATION)) + 1;
			qreal axisMaxX = LEFT_MARGIN + segmentCount * AXIS_DEFAULT_TICK_SEPARATION;
			scene->setSceneRect( 0, 0, axisMaxX + RIGHT_MARGIN, scene->height() );
			
			treeFile.close();
			headFile.close();
		}
	}
}
