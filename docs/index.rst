scenechange
===========

.. include:: badges.rst

Scene change detection plugin for VapourSynth, distributed as the ``vapoursynth-scenechange``
package with both compiled plugins bundled.

Installation
------------

.. code-block:: shell

   pip install vapoursynth-scenechange

The wheel installs the compiled ``scenechange`` (namespace ``scd``) and ``temporalsoften2``
(namespace ``focus2``) plugins into ``<site-packages>/vapoursynth/plugins``, which VapourSynth
autoloads. No separate plugin installation or ``LoadPlugin`` call is required.

Usage
-----

.. code-block:: python

   import vapoursynth as vs
   from scenechange import TemporalSoften

   core = vs.core
   clip = TemporalSoften(core).soften(clip, luma_threshold=4)

API
---

.. autoclass:: scenechange.TemporalSoften
   :members:

.. only:: html

   Indices and tables
   ==================

   * :ref:`genindex`
   * :ref:`modindex`
