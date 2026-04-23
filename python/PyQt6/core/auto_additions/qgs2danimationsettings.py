# The following has been generated automatically from src/core/qgs2danimationsettings.h
try:
    Qgs2DAnimationKeyFrame.__attribute_docs__ = {'time': 'Relative time of the keyframe in seconds', 'center': 'Center point of the map', 'scale': 'Map scale denominator, e.g. 1000.0 for a 1:1000 map.', 'rotation': 'Map rotation'}
    Qgs2DAnimationKeyFrame.__annotations__ = {'time': float, 'center': 'QgsPointXY', 'scale': float, 'rotation': float}
    Qgs2DAnimationKeyFrame.__doc__ = """Represents a key frame in a 2D animation.

.. versionadded:: 4.2"""
except (NameError, AttributeError):
    pass
try:
    Qgs2DAnimationSettings.__overridden_methods__ = ['readXml', 'writeXml', 'interpolateKeyFrame']
except (NameError, AttributeError):
    pass
