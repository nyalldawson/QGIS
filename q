--- python/core/auto_generated/symbology/qgslinesymbollayer.sip.in	2023-10-05 19:34:59.181041799 +1000
+++ python/core/auto_generated/symbology/qgslinesymbollayer.sip.in.e85792e572c297651154270b0fde45e51673a8d9.prepare	2023-11-04 14:00:50.154745508 +1000
@@ -1499,6 +1499,111 @@
 };
 
 
+class QgsFilledLineSymbolLayer : QgsLineSymbolLayer
+{
+%Docstring(signature="appended")
+
+A line symbol layer type which fills a stroked line with a :py:class:`QgsFillSymbol`.
+
+.. versionadded:: 3.36
+%End
+
+%TypeHeaderCode
+#include "qgslinesymbollayer.h"
+%End
+  public:
+
+    QgsFilledLineSymbolLayer( double width = DEFAULT_SIMPLELINE_WIDTH, QgsFillSymbol *fillSymbol /Transfer/ = 0 );
+%Docstring
+Constructor for QgsFilledLineSymbolLayer.
+
+If a ``fillSymbol`` is specified, it will be transferred to the symbol layer and used
+to fill the inside of the stroked line. If no ``fillSymbol`` is specified then a default
+symbol will be used.
+%End
+    ~QgsFilledLineSymbolLayer();
+
+    static QgsSymbolLayer *create( const QVariantMap &properties = QVariantMap() ) /Factory/;
+%Docstring
+Creates a new QgsFilledLineSymbolLayer, using the settings
+serialized in the ``properties`` map (corresponding to the output from
+:py:func:`QgsFilledLineSymbolLayer.properties()` ).
+%End
+
+    virtual QString layerType() const;
+
+    virtual void startRender( QgsSymbolRenderContext &context );
+
+    virtual void stopRender( QgsSymbolRenderContext &context );
+
+    virtual void renderPolyline( const QPolygonF &points, QgsSymbolRenderContext &context );
+
+    virtual QVariantMap properties() const;
+
+    virtual QgsFilledLineSymbolLayer *clone() const /Factory/;
+
+    virtual QgsSymbol *subSymbol();
+
+    virtual bool setSubSymbol( QgsSymbol *symbol /Transfer/ );
+
+    virtual bool hasDataDefinedProperties() const;
+
+    virtual void setColor( const QColor &c );
+
+    virtual QColor color() const;
+
+    virtual void setOutputUnit( Qgis::RenderUnit unit );
+
+    virtual Qgis::RenderUnit outputUnit() const;
+
+    virtual bool usesMapUnits() const;
+
+    virtual void setMapUnitScale( const QgsMapUnitScale &scale );
+
+    virtual QgsMapUnitScale mapUnitScale() const;
+
+    virtual double estimateMaxBleed( const QgsRenderContext &context ) const;
+
+    virtual QSet<QString> usedAttributes( const QgsRenderContext &context ) const;
+
+
+    Qt::PenJoinStyle penJoinStyle() const;
+%Docstring
+Returns the pen join style used to render the line (e.g. miter, bevel, round, etc).
+
+.. seealso:: :py:func:`setPenJoinStyle`
+%End
+
+    void setPenJoinStyle( Qt::PenJoinStyle style );
+%Docstring
+Sets the pen join ``style`` used to render the line (e.g. miter, bevel, round, etc).
+
+.. seealso:: :py:func:`penJoinStyle`
+%End
+
+    Qt::PenCapStyle penCapStyle() const;
+%Docstring
+Returns the pen cap style used to render the line (e.g. flat, square, round, etc).
+
+.. seealso:: :py:func:`setPenCapStyle`
+%End
+
+    void setPenCapStyle( Qt::PenCapStyle style );
+%Docstring
+Sets the pen cap ``style`` used to render the line (e.g. flat, square, round, etc).
+
+.. seealso:: :py:func:`penCapStyle`
+%End
+
+  protected:
+
+    QgsFilledLineSymbolLayer( const QgsFilledLineSymbolLayer & );
+
+
+
+};
+
+
 
 /************************************************************************
  * This file has been generated automatically from                      *
