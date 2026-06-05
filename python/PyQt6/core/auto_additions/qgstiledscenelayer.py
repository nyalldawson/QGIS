# The following has been generated automatically from src/core/tiledscene/qgstiledscenelayer.h
try:
    QgsTiledSceneLayer.LayerOptions.__attribute_docs__ = {'transformContext': 'Coordinate transform context', 'loadDefaultStyle': 'Set to ``True`` if the default layer style should be loaded', 'skipCrsValidation': "Controls whether the layer is allowed to have an invalid/unknown CRS.\n\nIf ``True``, then no validation will be performed on the layer's CRS and the layer\nlayer's :py:func:`~QgsTiledSceneLayer.crs` may be :py:func:`~QgsTiledSceneLayer.invalid` (i.e. the layer will have no georeferencing available\nand will be treated as having purely numerical coordinates).\n\nIf ``False`` (the default), the layer's CRS will be validated using :py:func:`QgsCoordinateReferenceSystem.validate()`,\nwhich may cause a blocking, user-facing dialog asking users to manually select the correct CRS for the\nlayer.", 'preferredCrs': 'An ordered list of preferable coordinate reference systems to use for the data provider.\n\nData providers with backends that supply data in a range of coordinate reference systems may use this\nto select an appropriate default CRS to use.\n\n.. versionadded:: 4.2'}
    QgsTiledSceneLayer.LayerOptions.__annotations__ = {'transformContext': 'QgsCoordinateTransformContext', 'loadDefaultStyle': bool, 'skipCrsValidation': bool, 'preferredCrs': 'List[QgsCoordinateReferenceSystem]'}
    QgsTiledSceneLayer.LayerOptions.__doc__ = """Setting options for loading tiled scene layers."""
    QgsTiledSceneLayer.LayerOptions.__group__ = ['tiledscene']
except (NameError, AttributeError):
    pass
try:
    QgsTiledSceneLayer.__overridden_methods__ = ['clone', 'extent', 'dataProvider', 'readXml', 'writeXml', 'readSymbology', 'readStyle', 'writeSymbology', 'writeStyle', 'setTransformContext', 'encodedSource', 'decodedSource', 'loadDefaultStyle', 'htmlMetadata', 'createMapRenderer', 'loadDefaultMetadata', 'elevationProperties', 'properties']
    QgsTiledSceneLayer.__group__ = ['tiledscene']
except (NameError, AttributeError):
    pass
