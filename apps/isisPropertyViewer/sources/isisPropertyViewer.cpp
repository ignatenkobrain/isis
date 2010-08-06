/****************************************************************
 *
 * Copyright (C) <year> Max Planck Institute for Human Cognitive and Brain Sciences, Leipzig
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Author: Erik Tuerke, tuerke@cbs.mpg.de, 2010
 *
 *****************************************************************/


#include <QtGui>
#include "isisPropertyViewer.hpp"

#include <stdlib.h>

isisPropertyViewer::isisPropertyViewer( const isis::util::slist &fileList, QMainWindow *parent )
	: QMainWindow( parent )
{
	//  isis::util::DefaultMsgPrint::stopBelow( isis::warning );
	ui.setupUi( this );
	this->ui.actionSave->setEnabled( false );
	this->ui.actionSaveAs->setEnabled( false );
	//connect itemDoubleClicked
	QObject::connect( this->ui.treeWidget, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ), this, SLOT( edit_item( QTreeWidgetItem *, int ) ) );
	this->ui.treeWidget->setColumnCount( 3 );
	QStringList header;
	header << tr( "Property" ) << tr( "Value" ) << tr( "Type" );
	this->ui.treeWidget->setHeaderLabels( header );

	for ( isis::util::slist::const_iterator fileIterator = fileList.begin(); fileIterator != fileList.end(); fileIterator++ ) {
		addFileToTree( tr( fileIterator->c_str() ) );
	}
}

Qt::ItemFlags isisPropertyViewer::flags( const QModelIndex &index ) const
{
	if ( not index.isValid() ) {
		return 0;
	}

	return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void isisPropertyViewer::on_action_Close_activated()
{
	this->close();
}

void isisPropertyViewer::on_action_Open_activated()
{
	QString fileName = QFileDialog::getOpenFileName( this, tr( "Open Image" ), QDir::currentPath(), "*.nii *.v" );
	addFileToTree( fileName );
}

void isisPropertyViewer::on_action_Clear_activated()
{
	this->ui.treeWidget->clear();
}

void isisPropertyViewer::on_actionSave_activated()
{
	m_propHolder.saveIt( "" , false );
	this->ui.actionSave->setEnabled( false );
	this->ui.actionSaveAs->setEnabled( false );
}

void  isisPropertyViewer::on_actionSaveAs_activated()
{
	QString fileName = QFileDialog::getSaveFileName( this, tr( "Save Image As" ), QDir::currentPath(), "*.nii *.v" );

	if( not fileName.toStdString().empty() ) {
		m_propHolder.saveIt( fileName , true );
		this->ui.actionSave->setEnabled( false );
		this->ui.actionSaveAs->setEnabled( false );
	}
}

void isisPropertyViewer::addFileToTree( const QString &fileName )
{
	if ( not fileName.isEmpty() ) {
		this->setStatusTip( fileName );
		isis::data::ImageList inputImageList = isis::data::IOFactory::load( fileName.toStdString(), "" );
		std::cout << "Loaded " << inputImageList.size() << " Images" << std::endl;
		BOOST_FOREACH( isis::data::ImageList::const_reference ref, inputImageList ) {
			createTree( ref, fileName );
			m_propHolder.addPropMapFromImage( ref, fileName );
		}

		if ( inputImageList.empty() ) {
			LOG( isis::DataLog, isis::error ) << "Input image list is empty!";
			QMessageBox::information( this, "Error", "Input image list is empty!" );
		}
	}
}
void isisPropertyViewer::createTree( const boost::shared_ptr<isis::data::Image> image, const QString &fileName )
{
	QString headerProp, headerVal;
	QStringList header;
	headerProp.sprintf( "Image" );
	headerVal = fileName;
	header << headerProp << headerVal;
	QTreeWidgetItem *headItem = new QTreeWidgetItem( header );
	this->ui.treeWidget->addTopLevelItem( headItem );
	m_keyList = image->getKeys();
	BOOST_FOREACH( PropKeyListType::const_reference ref, m_keyList ) {
		addPropToTree( image, ref, headItem );
	}
	unsigned short chunkCounter = 0;

	//go through all the chunks
	// for ( isis::data::Image::ChunkIterator chunkIterator = image->chunksBegin(); chunkIterator != image->chunksEnd(); chunkIterator++ ) {
	BOOST_FOREACH(boost::shared_ptr<isis::data::Chunk> chunkref, image->getChunkList()) {
		m_keyList = chunkref->getKeys();

		if ( not m_keyList.empty() ) {
			QString headerProp, headerVal;
			QStringList header;
			headerProp.sprintf( "Chunk" );
			headerVal.sprintf( "%d", chunkCounter );
			header << headerProp << headerVal;
			QTreeWidgetItem *chunkItem = new QTreeWidgetItem( header );
			headItem->addChild( chunkItem );
			BOOST_FOREACH( PropKeyListType::const_reference ref, m_keyList ) {
				addPropToTree( image, ref, chunkItem );
			}
		}

		chunkCounter++;
	} // END BOOST_FOREACH

	QString chunkString;
	chunkString.sprintf( "%d", chunkCounter );
	addChildToItem( headItem, tr( "chunks" ), chunkString, tr( "" ) );
}

void isisPropertyViewer::addChildToItem( QTreeWidgetItem *item, const QString &prop, const QString &val, const QString &type ) const
{
	QStringList stringList;
	stringList << prop << val << type;
	QTreeWidgetItem *newItem = new QTreeWidgetItem( stringList );
	item->addChild( newItem );
}

void isisPropertyViewer::addPropToTree( const boost::shared_ptr<isis::data::Image> image, PropKeyListType::const_reference propIterator, QTreeWidgetItem *currentHeadItem )
{
	LOG_IF( image->propertyValue( propIterator ).empty(), isis::CoreLog, isis::error ) << "Property " << propIterator << " is empty";

	if ( not image->propertyValue( propIterator ).empty() ) {
		if ( image->propertyValue( propIterator )->is<isis::util::fvector4>() ) {
			std::vector<QString> stringVec;
			QTreeWidgetItem *vectorItem = new QTreeWidgetItem( QStringList( tr( propIterator.c_str() ) ) );
			currentHeadItem->addChild( vectorItem );

			for ( unsigned short dim = 0; dim < 4; dim++ ) {
				QString tmp = "";
				tmp.sprintf( "%f", image->getProperty<isis::util::fvector4>( propIterator )[dim] );
				stringVec.push_back( tmp );
			}

			addChildToItem( vectorItem, tr( "x" ), stringVec[0], tr( "float" ) );
			addChildToItem( vectorItem, tr( "y" ), stringVec[1], tr( "float" ) );
			addChildToItem( vectorItem, tr( "z" ), stringVec[2], tr( "float" ) );
			addChildToItem( vectorItem, tr( "t" ), stringVec[3], tr( "float" ) );
		} else {
			addChildToItem( currentHeadItem, tr( propIterator.c_str() ),
							tr( image->propertyValue( propIterator )->toString( false ).c_str() ),
							tr( image->propertyValue( propIterator )->typeName().c_str() ) );
		}
	} else {
		addChildToItem( currentHeadItem, tr( propIterator.c_str() ), tr( "empty" ) , tr( "none" ) );
	}
};

void isisPropertyViewer::edit_item( QTreeWidgetItem *item, int val )
{
	//only edit value columns
	if ( val == 1 and ( not item->text( 2 ).toStdString().empty() ) ) {
		//      QMessageBox::information( this, "isisPropertyViewer", "Edit mode not yet implemented!"
		bool ok;
		QTreeWidgetItem *tmpItem = item;
		QString val = QInputDialog::getText( this, item->parent()->text( 0 ), item->text( 0 ), QLineEdit::Normal, item->text( 1 ), &ok );
		QString count = "";

		if ( ok ) {
			std::string currentFileName;

			// get the most parent item
			while ( count != tr( "Image" ) ) {
				tmpItem = tmpItem->parent();
				count = tmpItem->text( 0 );
			}

			m_propHolder.m_propChanged.find( tmpItem->text( 1 ).toStdString() )->second = true;
			currentFileName = tmpItem->text( 1 ).toStdString();
			isis::util::PropMap &tmpPropMap = m_propHolder.m_propHolderMap.find( currentFileName )->second;
			tmpItem = item;

			//go up to the next not empty prop if necessary. This might be the case if current item is a vector
			while ( not tmpPropMap.hasProperty( tmpItem->text( 0 ).toStdString() ) ) {
				tmpItem = tmpItem->parent();
			}

			std::string propName = tmpItem->text( 0 ).toStdString();
			const isis::util::PropertyValue &tmpProp = m_propHolder.m_propHolderMap.find( currentFileName )->second.propertyValue( propName );

			if ( not tmpProp->is<isis::util::fvector4>() ) {
				isis::util::Type<std::string> myVal( val.toStdString() );
				isis::util::_internal::TypeBase::convert( myVal, *tmpProp );
			} else {
				isis::util::fvector4 &tmpVector = tmpPropMap.propertyValue( propName )->cast_to_Type<isis::util::fvector4>();

				//TODO no method to get line of the current child item???
				if ( item->text( 0 ).toStdString() == "x" ) {
					tmpVector[0] = val.toFloat();
					std::cout << "tmpVector" << tmpVector << std::endl;
				} else if ( item->text( 0 ).toStdString() == "y" ) {
					tmpVector[1] = val.toFloat();
					std::cout << "tmpVector" << tmpVector << std::endl;
				} else if ( item->text( 0 ).toStdString() == "z" ) {
					tmpVector[2] = val.toFloat();
					std::cout << "tmpVector" << tmpVector << std::endl;
				} else if ( item->text( 0 ).toStdString() == "t" ) {
					tmpVector[3] = val.toFloat();
					std::cout << "tmpVector" << tmpVector << std::endl;
				}

				m_propHolder.m_propHolderMap.find( currentFileName )->second.propertyValue( propName ) = tmpVector ;
			}

			item->setText( 1, val );
			this->ui.actionSave->setEnabled( true );
			this->ui.actionSaveAs->setEnabled( true );
		}
	}
}


