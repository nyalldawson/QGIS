"""QGIS Unit tests for labeling engine rules

.. note:: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
"""

from qgis.core import (
    QgsLabelingEngineRuleRegistry,
    QgsAbstractLabelingEngineRule
)
import unittest
from qgis.testing import start_app, QgisTestCase

start_app()


class TestRule(QgsAbstractLabelingEngineRule):

    def id(self):
        return 'test'

    def prepare(self, context):
        pass

    def writeXml(self, doc, element, context):
        pass

    def modifyProblem(self):
        pass

    def readXml(self, element, context):
        pass

    def clone(self):
        return TestRule()


class TestQgsLabelingEngineRule(QgisTestCase):

    def testRegistry(self):
        registry = QgsLabelingEngineRuleRegistry()
        self.assertTrue(registry.ruleIds())
        for rule_id in registry.ruleIds():
            self.assertEqual(registry.create(rule_id).id(), rule_id)

        self.assertIsNone(registry.create('bad'))

        self.assertIn('minimumDistanceLabelToFeature', registry.ruleIds())

        self.assertFalse(registry.addRule(None))

        self.assertTrue(registry.addRule(TestRule()))

        self.assertIn('test', registry.ruleIds())
        self.assertTrue(isinstance(registry.create('test'), TestRule))

        # no duplicates
        self.assertFalse(registry.addRule(TestRule()))

        registry.removeRule('test')

        self.assertNotIn('test', registry.ruleIds())
        self.assertIsNone(registry.create('test'))

        registry.removeRule('test')


if __name__ == '__main__':
    unittest.main()
