# The following has been generated automatically from src/core/providers/qgsprovidersublayerdetails.h
try:
    QgsProviderSublayerDetails.LayerOptions.__attribute_docs__ = {'transformContext': 'Coordinate transform context', 'loadDefaultStyle': 'Set to ``True`` if the default layer style should be loaded', 'loadAllStoredStyle': "Controls whether the stored styles will be all loaded.\n\nIf ``True`` and the layer's provider supports style stored in the\ndata source all the available styles will be loaded in addition\nto the default one.\n\nIf ``False`` (the default), the layer's provider will only load\nthe default style.\n\n.. versionadded:: 3.30", 'preferredCrs': 'An ordered list of preferable coordinate reference systems to use for the data provider.\n\nData providers with backends that supply data in a range of coordinate reference systems may use this\nto select an appropriate default CRS to use.\n\n.. versionadded:: 4.2'}
    QgsProviderSublayerDetails.LayerOptions.__annotations__ = {'transformContext': 'QgsCoordinateTransformContext', 'loadDefaultStyle': bool, 'loadAllStoredStyle': bool, 'preferredCrs': 'List[QgsCoordinateReferenceSystem]'}
    QgsProviderSublayerDetails.LayerOptions.__doc__ = """Setting options for loading layers."""
    QgsProviderSublayerDetails.LayerOptions.__group__ = ['providers']
except (NameError, AttributeError):
    pass
try:
    QgsProviderSublayerDetails.__group__ = ['providers']
except (NameError, AttributeError):
    pass
