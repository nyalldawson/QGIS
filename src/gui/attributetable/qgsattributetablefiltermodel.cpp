/***************************************************************************
     QgsAttributeTableFilterModel.cpp
     --------------------------------------
    Date                 : Feb 2009
    Copyright            : (C) 2009 Vita Cizek
    Email                : weetya (at) gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsattributetablefiltermodel.h"

#include "qgis.h"
#include "qgsattributetablemodel.h"
#include "qgsexpressioncontextutils.h"
#include "qgsfeature.h"
#include "qgsfeatureiterator.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmapcanvasutils.h"
#include "qgsrendercontext.h"
#include "qgsrenderer.h"
#include "qgsvectorlayer.h"
#include "qgsvectorlayercache.h"
#include "qgsvectorlayereditbuffer.h"
#include "qgsvectorlayertemporalproperties.h"

#include <QApplication>
#include <QItemSelectionModel>
#include <QString>

#ifdef ENABLE_MODELTEST
#include <QAbstractItemModelTester>
#endif

#include "moc_qgsattributetablefiltermodel.cpp"

using namespace Qt::StringLiterals;

//
// QgsAttributeTableReorderedColumnModel
//

QgsAttributeTableReorderedColumnModel::QgsAttributeTableReorderedColumnModel( QgsAttributeTableModel *tableModel, QObject *parent )
  : QIdentityProxyModel( parent )
  , mTableModel( tableModel )
{
  for ( int i = 0; i < mTableModel->columnCount(); ++i )
  {
    mProxyColumnSource.append( i );
  }

  setSourceModel( mTableModel );
  connect( mTableModel, &QgsAttributeTableModel::layoutAboutToBeChanged, this, &QgsAttributeTableReorderedColumnModel::sourceLayoutAboutToBeChanged );
  connect( mTableModel, &QgsAttributeTableModel::layoutChanged, this, &QgsAttributeTableReorderedColumnModel::sourceLayoutChanged );
}

int QgsAttributeTableReorderedColumnModel::columnCount( const QModelIndex &parent ) const
{
  if ( parent.isValid() )
    return 0;

  return mProxyColumnSource.count();
}

QVariant QgsAttributeTableReorderedColumnModel::data( const QModelIndex &index, int role ) const
{
  const int attributeIndex = proxyColumnToAttributeIndex( index.column() );
  if ( attributeIndex == -1 ) // actions column
  {
    if ( role == static_cast<int>( QgsAttributeTableFilterModel::CustomRole::Type ) )
      return QgsAttributeTableFilterModel::ColumnTypeActionButton;
    else if ( role == static_cast<int>( QgsAttributeTableModel::CustomRole::FeatureId ) )
    {
      const QModelIndex fieldIndex = mapToSource( QIdentityProxyModel::index( index.row(), 0, index.parent() ) );
      return sourceModel()->data( fieldIndex, static_cast<int>( QgsAttributeTableModel::CustomRole::FeatureId ) );
    }
    else if ( role == static_cast<int>( QgsAttributeTableModel::CustomRole::Sort ) )
    {
      const QModelIndex fieldIndex = mapToSource( QIdentityProxyModel::index( index.row(), 0, index.parent() ) );
      return sourceModel()->data( fieldIndex, static_cast<int>( QgsAttributeTableModel::CustomRole::Sort ) );
    }
  }
  else if ( role == static_cast<int>( QgsAttributeTableFilterModel::CustomRole::Type ) )
  {
    return QgsAttributeTableFilterModel::ColumnTypeField;
  }

  return QIdentityProxyModel::data( index, role );
}

Qt::ItemFlags QgsAttributeTableReorderedColumnModel::flags( const QModelIndex &index ) const
{
  if ( !index.isValid() )
    return Qt::ItemFlag::NoItemFlags;

  // Handle the action column flags here, the master model doesn't know it
  const int attributeIndex = proxyColumnToAttributeIndex( index.column() );
  if ( attributeIndex == -1 )
  {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }

  return QIdentityProxyModel::flags( index );
}

QVariant QgsAttributeTableReorderedColumnModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation == Qt::Horizontal )
  {
    const int attributeIndex = proxyColumnToAttributeIndex( section );
    if ( attributeIndex == -1 )
    {
      if ( role == Qt::DisplayRole )
      {
        return tr( "Actions" );
      }
      else
      {
        return QVariant();
      }
    }
    else
    {
      return sourceModel()->headerData( attributeIndex, orientation, role );
    }
  }
  else
  {
    if ( role == Qt::DisplayRole )
    {
      return section + 1;
    }
    else
    {
      return sourceModel()->headerData( section, orientation, role );
    }
  }
}

QModelIndex QgsAttributeTableReorderedColumnModel::index( int row, int column, const QModelIndex &parent ) const
{
  if ( row < 0 || column < 0 || column >= columnCount( parent ) )
    return QModelIndex();

  return createIndex( row, column );
}

QModelIndex QgsAttributeTableReorderedColumnModel::parent( const QModelIndex &child ) const
{
  const int attributeIndex = proxyColumnToAttributeIndex( child.column() );
  if ( attributeIndex < 0 )
  {
    // Create an index for column 0 and use that to get the parent.
    const QModelIndex proxySibling = createIndex( child.row(), 0, child.internalPointer() );
    return QIdentityProxyModel::parent( proxySibling );
  }
  return QIdentityProxyModel::parent( child );
}

QModelIndex QgsAttributeTableReorderedColumnModel::mapToSource( const QModelIndex &proxyIndex ) const
{
  if ( !proxyIndex.isValid() )
    return QModelIndex();

  const int attributeIndex = proxyColumnToAttributeIndex( proxyIndex.column() );
  if ( attributeIndex < 0 )
  {
    // does not exist in the source model
    return QModelIndex();
  }

  return mTableModel->index( proxyIndex.row(), attributeIndex );
}

QItemSelection QgsAttributeTableReorderedColumnModel::mapSelectionToSource( const QItemSelection &selection ) const
{
  QItemSelection sourceSelection;
  // mapToSource will give invalid index for our additional columns, so truncate the selection
  // to the columns known by the source model
  for ( auto it = selection.constBegin(); it != selection.constEnd(); ++it )
  {
    QItemSelection newSelection;
    for ( const QModelIndex &proxyIndex : it->indexes() )
    {
      const QModelIndex sourceIndex = mapToSource( proxyIndex );
      if ( sourceIndex.isValid() )
      {
        newSelection << QItemSelectionRange( sourceIndex, sourceIndex );
      }
    }
    sourceSelection.merge( newSelection, QItemSelectionModel::Select );
  }

  return sourceSelection;
}

QModelIndex QgsAttributeTableReorderedColumnModel::mapFromSource( const QModelIndex &sourceIndex ) const
{
  if ( !sourceIndex.isValid() )
    return QModelIndex();

  const int proxyColumn = attributeIndexToProxyColumn( sourceIndex.column() );
  if ( proxyColumn < 0 )
  {
    // attribute does not exist in this model
    return QModelIndex();
  }

  return createIndex( sourceIndex.row(), proxyColumn );
}

QModelIndex QgsAttributeTableReorderedColumnModel::sibling( int row, int column, const QModelIndex &idx ) const
{
  if ( row == idx.row() && column == idx.column() )
  {
    return idx;
  }
  return index( row, column, parent( idx ) );
}

QModelIndex QgsAttributeTableReorderedColumnModel::buddy( const QModelIndex &proxyIndex ) const
{
  const int attributeIndex = proxyColumnToAttributeIndex( proxyIndex.column() );
  if ( attributeIndex < 0 )
  {
    // does not exist in the source model
    return proxyIndex;
  }

  return QIdentityProxyModel::buddy( proxyIndex );
}

void QgsAttributeTableReorderedColumnModel::setAttributeTableConfig( const QgsAttributeTableConfig &config )
{
  const QgsAttributeTableConfig oldConfig = mConfig;
  mConfig = config;
  QgsVectorLayer *layer = mTableModel->layer();

  QVector<int> newColumnMapping;
  QSet<int> newHiddenColumns;
  const QVector<QgsAttributeTableConfig::ColumnConfig> constColumns = mConfig.columns();
  for ( const QgsAttributeTableConfig::ColumnConfig &columnConfig : constColumns )
  {
    // The new value for the mapping (field index or -1 for action column)
    switch ( columnConfig.type )
    {
      case QgsAttributeTableConfig::Field:
      {
        const int fieldIndex = layer->fields().lookupField( columnConfig.name );
        if ( fieldIndex >= 0 )
        {
          if ( columnConfig.hidden )
          {
            newHiddenColumns.insert( fieldIndex );
          }
          else
          {
            newColumnMapping << fieldIndex;
          }
        }
        break;
      }

      case QgsAttributeTableConfig::Action:
        if ( !columnConfig.hidden )
        {
          newColumnMapping << -1;
        }
        break;
    }
  }

  if ( newColumnMapping != mProxyColumnSource )
  {
    bool requiresReset = false;
    int firstRemovedColumn = -1;
    int removedColumnCount = 0;
    mHiddenColumns = newHiddenColumns;

    // Check if there have a contiguous set of columns have been removed or if we require a full reset
    for ( int i = 0; i < std::min( newColumnMapping.size(), mProxyColumnSource.size() - removedColumnCount ); ++i )
    {
      if ( newColumnMapping.at( i ) == mProxyColumnSource.at( i + removedColumnCount ) )
        continue;

      if ( firstRemovedColumn == -1 )
      {
        firstRemovedColumn = i;

        while ( i < mProxyColumnSource.size() - removedColumnCount && mProxyColumnSource.at( i + removedColumnCount ) != newColumnMapping.at( i ) )
        {
          ++removedColumnCount;
        }
      }
      else
      {
        requiresReset = true;
        break;
      }
    }

    // No difference found so far
    if ( firstRemovedColumn == -1 )
    {
      if ( newColumnMapping.size() > mProxyColumnSource.size() )
      {
        // More columns: appended to the end
        beginInsertColumns( QModelIndex(), mProxyColumnSource.size(), newColumnMapping.size() - 1 );
        mProxyColumnSource = newColumnMapping;
        endInsertColumns();
      }
      else
      {
        // Less columns: removed from the end
        beginRemoveColumns( QModelIndex(), newColumnMapping.size(), mProxyColumnSource.size() - 1 );
        mProxyColumnSource = newColumnMapping;
        endRemoveColumns();
      }
    }
    else
    {
      if ( newColumnMapping.size() == mProxyColumnSource.size() - removedColumnCount )
      {
        //the amount of removed column in the model need to be equal removedColumnCount
        beginRemoveColumns( QModelIndex(), firstRemovedColumn, firstRemovedColumn + removedColumnCount - 1 );
        mProxyColumnSource = newColumnMapping;
        endRemoveColumns();
      }
      else
      {
        requiresReset = true;
      }
    }

    if ( requiresReset )
    {
      beginResetModel();
      mProxyColumnSource = newColumnMapping;
      endResetModel();
    }
  }
}

int QgsAttributeTableReorderedColumnModel::proxyColumnToAttributeIndex( int column ) const
{
  return mProxyColumnSource.value( column, -1 );
}

int QgsAttributeTableReorderedColumnModel::attributeIndexToProxyColumn( int index ) const
{
  if ( mHiddenColumns.contains( index ) )
    return -1;

  return mProxyColumnSource.indexOf( index );
}

void QgsAttributeTableReorderedColumnModel::sourceLayoutAboutToBeChanged( const QList<QPersistentModelIndex> &sourceParents, LayoutChangeHint hint )
{
  QList<QPersistentModelIndex> parents;
  parents.reserve( sourceParents.size() );
  for ( const QPersistentModelIndex &parent : sourceParents )
  {
    if ( !parent.isValid() )
    {
      parents << QPersistentModelIndex();
      continue;
    }
    const QModelIndex mappedParent = mapFromSource( parent );
    parents << mappedParent;
  }

  emit layoutAboutToBeChanged( parents, hint );

  const QModelIndexList persistentIndexList = QIdentityProxyModel::persistentIndexList();
  layoutChangePersistentIndexes.reserve( persistentIndexList.size() );
  layoutChangeProxyColumns.reserve( persistentIndexList.size() );

  for ( QModelIndex proxyPersistentIndex : persistentIndexList )
  {
    proxyIndexes << proxyPersistentIndex;
    const int column = proxyPersistentIndex.column();
    layoutChangeProxyColumns << column;

    const int attributeIndex = proxyColumnToAttributeIndex( column );
    if ( attributeIndex < 0 )
    {
      proxyPersistentIndex = proxyPersistentIndex.sibling( proxyPersistentIndex.row(), 0 );
    }

    const QPersistentModelIndex srcPersistentIndex = mapToSource( proxyPersistentIndex );
    layoutChangePersistentIndexes << srcPersistentIndex;
  }
}

void QgsAttributeTableReorderedColumnModel::sourceLayoutChanged( const QList<QPersistentModelIndex> &sourceParents, LayoutChangeHint hint )
{
  for ( int i = 0; i < proxyIndexes.size(); ++i )
  {
    const QModelIndex proxyIdx = proxyIndexes.at( i );
    QModelIndex newProxyIdx = mapFromSource( layoutChangePersistentIndexes.at( i ) );
    const int attributeIndex = proxyColumnToAttributeIndex( proxyIdx.column() );
    if ( attributeIndex < 0 )
    {
      newProxyIdx = newProxyIdx.sibling( newProxyIdx.row(), layoutChangeProxyColumns.at( i ) );
    }
    changePersistentIndex( proxyIdx, newProxyIdx );
  }

  layoutChangePersistentIndexes.clear();
  layoutChangeProxyColumns.clear();
  proxyIndexes.clear();

  QList<QPersistentModelIndex> parents;
  parents.reserve( sourceParents.size() );
  for ( const QPersistentModelIndex &parent : sourceParents )
  {
    if ( !parent.isValid() )
    {
      parents << QPersistentModelIndex();
      continue;
    }
    const QModelIndex mappedParent = mapFromSource( parent );
    parents << mappedParent;
  }

  emit layoutChanged( parents, hint );
}


//////////////////
// Filter Model //
//////////////////

QgsAttributeTableFilterModel::QgsAttributeTableFilterModel( QgsMapCanvas *canvas, QgsAttributeTableModel *sourceModel, QObject *parent )
  : QSortFilterProxyModel( parent )
  , mCanvas( canvas )
{
  setSourceModel( sourceModel );

  setDynamicSortFilter( true );
  setSortRole( static_cast<int>( QgsAttributeTableModel::CustomRole::Sort ) );
  connect( layer(), &QgsVectorLayer::selectionChanged, this, &QgsAttributeTableFilterModel::selectionChanged );

  mReloadVisibleTimer.setSingleShot( true );
  connect( &mReloadVisibleTimer, &QTimer::timeout, this, &QgsAttributeTableFilterModel::reloadVisible );
  mFilterFeaturesTimer.setSingleShot( true );
  connect( &mFilterFeaturesTimer, &QTimer::timeout, this, &QgsAttributeTableFilterModel::filterFeatures );
}

bool QgsAttributeTableFilterModel::lessThan( const QModelIndex &left, const QModelIndex &right ) const
{
  if ( mSelectedOnTop )
  {
    const bool leftSelected = layer()->selectedFeatureIds().contains( masterModel()->rowToId( left.row() ) );
    const bool rightSelected = layer()->selectedFeatureIds().contains( masterModel()->rowToId( right.row() ) );

    if ( leftSelected && !rightSelected )
    {
      return sortOrder() == Qt::AscendingOrder;
    }
    else if ( rightSelected && !leftSelected )
    {
      return sortOrder() == Qt::DescendingOrder;
    }
  }

  if ( mTableModel->sortCacheExpression().isEmpty() )
  {
    //shortcut when no sort order set
    return false;
  }

  return qgsVariantLessThan( left.data( static_cast<int>( QgsAttributeTableModel::CustomRole::Sort ) ), right.data( static_cast<int>( QgsAttributeTableModel::CustomRole::Sort ) ) );
}

void QgsAttributeTableFilterModel::sort( int column, Qt::SortOrder order )
{
  if ( order != Qt::AscendingOrder && order != Qt::DescendingOrder )
    order = Qt::AscendingOrder;

  const int attributeIndexForColumn = mReorderedColumnModel->proxyColumnToAttributeIndex( column );
  if ( attributeIndexForColumn < 0 )
  {
    sort( QString() );
  }
  else
  {
    masterModel()->prefetchColumnData( attributeIndexForColumn );
    QSortFilterProxyModel::sort( column, order );
  }
  emit sortColumnChanged( column, order );
}

int QgsAttributeTableFilterModel::actionColumnIndex() const
{
  return mReorderedColumnModel->attributeIndexToProxyColumn( -1 );
}

void QgsAttributeTableFilterModel::setAttributeTableConfig( const QgsAttributeTableConfig &config, bool force )
{
  const QgsAttributeTableConfig oldConfig = mConfig;
  mConfig = config;
  mConfig.update( mTableModel->layer()->fields() );

  if ( !force && mConfig.hasSameColumns( oldConfig ) )
  {
    return;
  }

  mReorderedColumnModel->setAttributeTableConfig( mConfig );
  if ( !mConfig.sortExpression().isEmpty() )
  {
    sort( mConfig.sortExpression(), mConfig.sortOrder() );
  }
}

void QgsAttributeTableFilterModel::setFilterExpression( const QgsExpression &expression, const QgsExpressionContext &context )
{
  mFilterExpression = expression;
  mFilterExpressionContext = context;
}

void QgsAttributeTableFilterModel::sort( const QString &expression, Qt::SortOrder order )
{
  if ( order != Qt::AscendingOrder && order != Qt::DescendingOrder )
    order = Qt::AscendingOrder;

  QSortFilterProxyModel::sort( -1 );
  masterModel()->prefetchSortData( expression );
  QSortFilterProxyModel::sort( 0, order );
}

QString QgsAttributeTableFilterModel::sortExpression() const
{
  return masterModel()->sortCacheExpression();
}

void QgsAttributeTableFilterModel::setSelectedOnTop( bool selectedOnTop )
{
  if ( mSelectedOnTop != selectedOnTop )
  {
    mSelectedOnTop = selectedOnTop;
    int column = sortColumn();
    Qt::SortOrder order = sortOrder();

    // set default sort values if they are not correctly set
    if ( column < 0 )
      column = 0;

    if ( order != Qt::AscendingOrder && order != Qt::DescendingOrder )
      order = Qt::AscendingOrder;

    sort( column, order );
    invalidate();
  }
}

void QgsAttributeTableFilterModel::setSourceModel( QgsAttributeTableModel *sourceModel )
{
  mTableModel = sourceModel;
  mReorderedColumnModel = new QgsAttributeTableReorderedColumnModel( mTableModel, this );
#ifdef ENABLE_MODELTEST
  new QAbstractItemModelTester( mReorderedColumnModel, QAbstractItemModelTester::FailureReportingMode::Fatal, this );
#endif

  QSortFilterProxyModel::setSourceModel( mReorderedColumnModel );

  // The following connections are needed in order to keep the filter model in sync, see: regression #15974
  connect( mTableModel, &QAbstractItemModel::columnsAboutToBeInserted, this, &QgsAttributeTableFilterModel::onColumnsChanged );
  connect( mTableModel, &QAbstractItemModel::columnsAboutToBeRemoved, this, &QgsAttributeTableFilterModel::onColumnsChanged );
}

bool QgsAttributeTableFilterModel::selectedOnTop()
{
  return mSelectedOnTop;
}

void QgsAttributeTableFilterModel::setFilteredFeatures( const QgsFeatureIds &ids )
{
  mFilteredFeatures = ids;
  if ( mFilterMode != ShowFilteredList && mFilterMode != ShowInvalid )
  {
    setFilterMode( ShowFilteredList );
  }
  invalidateFilter();
}

QgsFeatureIds QgsAttributeTableFilterModel::filteredFeatures()
{
  QgsFeatureIds ids;
  ids.reserve( rowCount() );
  for ( int i = 0; i < rowCount(); ++i )
  {
    const QModelIndex row = index( i, 0 );
    ids << rowToId( row );
  }
  return ids;
}

void QgsAttributeTableFilterModel::setFilterMode( FilterMode filterMode )
{
  if ( filterMode != mFilterMode )
  {
    disconnectFilterModeConnections();
    connectFilterModeConnections( filterMode );
    mFilterMode = filterMode;
    invalidate();

    if ( mFilterMode == QgsAttributeTableFilterModel::ShowInvalid )
    {
      filterFeatures();
    }
  }
}

void QgsAttributeTableFilterModel::disconnectFilterModeConnections()
{
  // cleanup existing connections
  switch ( mFilterMode )
  {
    case ShowVisible:
      disconnect( mCanvas, &QgsMapCanvas::extentsChanged, this, &QgsAttributeTableFilterModel::startTimedReloadVisible );
      disconnect( mCanvas, &QgsMapCanvas::temporalRangeChanged, this, &QgsAttributeTableFilterModel::startTimedReloadVisible );
      disconnect( layer(), &QgsVectorLayer::featureAdded, this, &QgsAttributeTableFilterModel::startTimedReloadVisible );
      disconnect( layer(), &QgsVectorLayer::geometryChanged, this, &QgsAttributeTableFilterModel::startTimedReloadVisible );
      break;
    case ShowAll:
    case ShowEdited:
    case ShowSelected:
      break;
    case ShowFilteredList:
    case ShowInvalid:
      disconnect( layer(), &QgsVectorLayer::featureAdded, this, &QgsAttributeTableFilterModel::startTimedFilterFeatures );
      disconnect( layer(), &QgsVectorLayer::attributeValueChanged, this, &QgsAttributeTableFilterModel::onAttributeValueChanged );
      disconnect( layer(), &QgsVectorLayer::geometryChanged, this, &QgsAttributeTableFilterModel::onGeometryChanged );
      break;
  }
}

void QgsAttributeTableFilterModel::connectFilterModeConnections( QgsAttributeTableFilterModel::FilterMode filterMode )
{
  // setup new connections
  switch ( filterMode )
  {
    case ShowVisible:
      connect( mCanvas, &QgsMapCanvas::extentsChanged, this, &QgsAttributeTableFilterModel::startTimedReloadVisible );
      connect( mCanvas, &QgsMapCanvas::temporalRangeChanged, this, &QgsAttributeTableFilterModel::startTimedReloadVisible );
      connect( layer(), &QgsVectorLayer::featureAdded, this, &QgsAttributeTableFilterModel::startTimedReloadVisible );
      connect( layer(), &QgsVectorLayer::geometryChanged, this, &QgsAttributeTableFilterModel::startTimedReloadVisible );
      generateListOfVisibleFeatures();
      break;
    case ShowAll:
    case ShowEdited:
    case ShowSelected:
      break;
    case ShowFilteredList:
    case ShowInvalid:
      connect( layer(), &QgsVectorLayer::featureAdded, this, &QgsAttributeTableFilterModel::startTimedFilterFeatures );
      connect( layer(), &QgsVectorLayer::attributeValueChanged, this, &QgsAttributeTableFilterModel::onAttributeValueChanged );
      connect( layer(), &QgsVectorLayer::geometryChanged, this, &QgsAttributeTableFilterModel::onGeometryChanged );
      break;
  }
}

bool QgsAttributeTableFilterModel::filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent ) const
{
  Q_UNUSED( sourceParent )
  switch ( mFilterMode )
  {
    case ShowAll:
      return true;

    case ShowFilteredList:
    case ShowInvalid:
      return mFilteredFeatures.contains( masterModel()->rowToId( sourceRow ) );

    case ShowSelected:
      return layer()->selectedFeatureIds().contains( masterModel()->rowToId( sourceRow ) );

    case ShowVisible:
      return mFilteredFeatures.contains( masterModel()->rowToId( sourceRow ) );

    case ShowEdited:
    {
      QgsVectorLayerEditBuffer *editBuffer = layer()->editBuffer();
      if ( editBuffer )
      {
        const QgsFeatureId fid = masterModel()->rowToId( sourceRow );

        if ( editBuffer->isFeatureAdded( fid ) )
          return true;

        if ( editBuffer->isFeatureAttributesChanged( fid ) )
          return true;

        if ( editBuffer->isFeatureGeometryChanged( fid ) )
          return true;

        return false;
      }
      return false;
    }

    default:
      Q_ASSERT( false ); // In debug mode complain
      return true;       // In release mode accept row
  }
  // returns are handled in their respective case statement above
}

void QgsAttributeTableFilterModel::extentsChanged()
{
  reloadVisible();
}

void QgsAttributeTableFilterModel::reloadVisible()
{
  generateListOfVisibleFeatures();
  invalidateFilter();
  emit visibleReloaded();
}

void QgsAttributeTableFilterModel::onAttributeValueChanged( QgsFeatureId fid, int idx, const QVariant &value )
{
  Q_UNUSED( fid );
  Q_UNUSED( value );

  if ( mFilterMode == QgsAttributeTableFilterModel::ShowInvalid || mFilterExpression.referencedAttributeIndexes( layer()->fields() ).contains( idx ) )
  {
    startTimedFilterFeatures();
  }
}

void QgsAttributeTableFilterModel::onGeometryChanged()
{
  if ( mFilterMode == QgsAttributeTableFilterModel::ShowInvalid || mFilterExpression.needsGeometry() )
  {
    startTimedFilterFeatures();
  }
}

void QgsAttributeTableFilterModel::startTimedReloadVisible()
{
  mReloadVisibleTimer.start( 10 );
}

void QgsAttributeTableFilterModel::startTimedFilterFeatures()
{
  mFilterFeaturesTimer.start( 10 );
}

void QgsAttributeTableFilterModel::filterFeatures()
{
  if ( !mFilterExpression.isValid() )
    return;

  QgsFeatureIds filteredFeatures;
  QgsDistanceArea distanceArea;

  distanceArea.setSourceCrs( mTableModel->layer()->crs(), QgsProject::instance()->transformContext() );
  distanceArea.setEllipsoid( QgsProject::instance()->ellipsoid() );

  const bool fetchGeom = mFilterExpression.needsGeometry();

  QApplication::setOverrideCursor( Qt::WaitCursor );

  mFilterExpression.setGeomCalculator( &distanceArea );
  mFilterExpression.setDistanceUnits( QgsProject::instance()->distanceUnits() );
  mFilterExpression.setAreaUnits( QgsProject::instance()->areaUnits() );
  QgsFeatureRequest request( mTableModel->request() );
  request.setSubsetOfAttributes( mFilterExpression.referencedColumns(), mTableModel->layer()->fields() );
  if ( !fetchGeom )
  {
    request.setFlags( Qgis::FeatureRequestFlag::NoGeometry );
  }
  else
  {
    // force geometry extraction if the filter requests it
    request.setFlags( request.flags() & ~static_cast<int>( Qgis::FeatureRequestFlag::NoGeometry ) );
  }
  QgsFeatureIterator featIt = mTableModel->layer()->getFeatures( request );

  QgsFeature f;

  // Record the first evaluation error
  QString error;

  while ( featIt.nextFeature( f ) )
  {
    mFilterExpressionContext.setFeature( f );
    if ( mFilterExpression.evaluate( &mFilterExpressionContext ).toInt() != 0 )
      filteredFeatures << f.id();

    // check if there were errors during evaluating
    if ( mFilterExpression.hasEvalError() && error.isEmpty() )
    {
      error = mFilterExpression.evalErrorString();
    }
  }

  featIt.close();

  setFilteredFeatures( filteredFeatures );

  QApplication::restoreOverrideCursor();

  emit featuresFiltered();

  if ( !error.isEmpty() )
  {
    emit filterError( error );
  }
}

void QgsAttributeTableFilterModel::selectionChanged()
{
  if ( ShowSelected == mFilterMode )
  {
    invalidateFilter();
  }
  else if ( mSelectedOnTop )
  {
    invalidate();
  }
}

void QgsAttributeTableFilterModel::onColumnsChanged()
{
  setAttributeTableConfig( mConfig );
}

void QgsAttributeTableFilterModel::generateListOfVisibleFeatures()
{
  if ( !layer() )
    return;

  bool filter = false;
  const QgsRectangle rect = mCanvas->mapSettings().mapToLayerCoordinates( layer(), mCanvas->extent() );
  QgsRenderContext renderContext;
  renderContext.expressionContext().appendScopes( QgsExpressionContextUtils::globalProjectLayerScopes( layer() ) );

  mFilteredFeatures.clear();
  if ( !layer()->renderer() )
  {
    QgsDebugError( u"Cannot get renderer"_s );
    return;
  }

  std::unique_ptr<QgsFeatureRenderer> renderer( layer()->renderer()->clone() );

  const QgsMapSettings &ms = mCanvas->mapSettings();
  if ( !layer()->isInScaleRange( ms.scale() ) )
  {
    QgsDebugMsgLevel( u"Out of scale limits"_s, 2 );
  }
  else
  {
    if ( renderer->capabilities() & QgsFeatureRenderer::ScaleDependent )
    {
      // setup scale
      // mapRenderer()->renderContext()->scale is not automatically updated when
      // render extent changes (because it's scale is used to identify if changed
      // since last render) -> use local context
      renderContext.setExtent( ms.visibleExtent() );
      renderContext.setMapToPixel( ms.mapToPixel() );
      renderContext.setRendererScale( ms.scale() );
    }

    filter = renderer->capabilities() & QgsFeatureRenderer::Filter;
  }

  renderer->startRender( renderContext, layer()->fields() );

  QgsFeatureRequest r( masterModel()->request() );
  if ( r.spatialFilterType() == Qgis::SpatialFilterType::BoundingBox )
  {
    r.setFilterRect( r.filterRect().intersect( rect ) );
  }
  else
  {
    r.setFilterRect( rect );
  }

  const QString canvasFilter = QgsMapCanvasUtils::filterForLayer( mCanvas, layer() );
  if ( canvasFilter == "FALSE"_L1 )
    return;
  if ( !canvasFilter.isEmpty() )
    r.setFilterExpression( canvasFilter );

  QgsFeatureIterator features = masterModel()->layerCache()->getFeatures( r );

  QgsFeature f;

  while ( features.nextFeature( f ) )
  {
    renderContext.expressionContext().setFeature( f );
    if ( !filter || renderer->willRenderFeature( f, renderContext ) )
    {
      mFilteredFeatures << f.id();
    }
#if 0
    if ( t.elapsed() > 5000 )
    {
      bool cancel = false;
      emit progress( i, cancel );
      if ( cancel )
        break;

      t.restart();
    }
#endif
  }

  features.close();

  if ( renderer->capabilities() & QgsFeatureRenderer::ScaleDependent )
  {
    renderer->stopRender( renderContext );
  }
}

QgsFeatureId QgsAttributeTableFilterModel::rowToId( const QModelIndex &row )
{
  return masterModel()->rowToId( mapToMaster( row ).row() );
}

QModelIndex QgsAttributeTableFilterModel::fidToIndex( QgsFeatureId fid )
{
  return mapFromMaster( masterModel()->idToIndex( fid ) );
}

QModelIndexList QgsAttributeTableFilterModel::fidToIndexList( QgsFeatureId fid )
{
  QModelIndexList indexes;
  const auto constIdToIndexList = masterModel()->idToIndexList( fid );
  for ( const QModelIndex &idx : constIdToIndexList )
  {
    indexes.append( mapFromMaster( idx ) );
  }

  return indexes;
}

QModelIndex QgsAttributeTableFilterModel::mapToMaster( const QModelIndex &proxyIndex ) const
{
  if ( !proxyIndex.isValid() )
    return QModelIndex();

  const QModelIndex columnModelIndex = QSortFilterProxyModel::mapToSource( proxyIndex );
  const int attributeIndex = mReorderedColumnModel->proxyColumnToAttributeIndex( columnModelIndex.column() );

  // For the action column there is no matching column in the source model, just return the first one
  // so we are still able to query for the feature id, the feature...
  if ( attributeIndex == -1 )
  {
    return mReorderedColumnModel->mapToSource( mReorderedColumnModel->index( columnModelIndex.row(), 0 ) );
  }
  else
  {
    return mReorderedColumnModel->mapToSource( columnModelIndex );
  }
}

QModelIndex QgsAttributeTableFilterModel::mapFromMaster( const QModelIndex &sourceIndex ) const
{
  return mapFromSource( mReorderedColumnModel->mapFromSource( sourceIndex ) );
}
