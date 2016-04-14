Ravel
=====
Ravel is a trace visualization tool for MPI with recent experimental support
for Charm++. Ravel is unique in that it shows not only physical timelines, but
also logical ones structured to better capture the intended organization of
communication operations. Ravel calculates logical structure from Open Trace
Format or Charm++ Projections logs and presents the results using multiple
coordinated views.

In logical time, all operations are colored via some metric. The default
metric for MPI is *lateness* which measures the difference in exit time of an
operation compared to its peers at the same logical timestep. 

![Ravel Logical and Physical Timelines](/images/pf3d32_sf_700.png)

Installation
------------
Ravel depends on:
- [Open Trace Format version 2 1.4+](http://www.vi-hps.org/projects/score-p/)
- [Qt4.8+](http://www.qt.io/download/)
- [Muster 1.0.1+](https://github.com/scalability-llnl/muster)
- (Install only) [cmake 2.8.9+](http://www.cmake.org/download/)
- (Optional) [Open Trace Format 1.12+](http://tu-dresden.de/die_tu_dresden/zentrale_einrichtungen/zih/forschung/projekte/otf/index_html/document_view?set_language=en)

To install:

    $ git clone https://github.com/scalability-llnl/ravel.git
    $ mkdir ravel/build
    $ cd ravel/build
    $ cmake -DCMAKE_INSTALL_PREFIX=/path/to/install/directory ..
    $ make
    $ make install

If a dependency is not found, add its install directory to the
`CMAKE_PREFIX_PATH` environment variable.

Usage
-----

### Opening a Trace

Before opening the trace, check your settings under `Options->OTF Importing`.
These options will affect the logical organization even determined by Ravel.
Once you are happy with your options, use `File->Open Trace` and navigate to
your `.otf`, `.otf2`, or `.sts` file.

#### Partitions

Ravel partitions the trace into fine-grained communication phases -- sets of
communication operations that must belong together. It imposes a
happened-before ordering between traces to better represent how developers
think of them separately.

* Automatically determine partitions: use happened-before relationships and
  the following options:
  * use Waitall heuristic: OTF version 1 only, will group all uninterrupted 
    send operations before each Waitall in the same phase
  * merge Partitions by call tree: Will merge communication operations up the
    call stack until reaching a call containing multiple such operations. MPI
    only.
  * merge Partitions to complete Leaps: Avoids sparse partitions by forcing
    each rank to be active at each distance in the phase DAG. Useful for bulk
    synchronous codes. MPI only.
    * skip Leaps that cannot be merged: Relaxes the leap merge when it cannot
      find a next merge.
  * merge across global steps: This merge happens after stepping, so it does
    not affect the logical structure, but groups MPI ranks that cover the same
    logical step. MPI only.
  * Charm++ break functions: Force the breaking of partitions at the given
    common-separated list of Charm++ entry methods.
* Partition at function breaks: Use if you know your phases are contained in
  a given function. List the function. 

#### Other Options
* Matching send/recv must have the same message size: Enforces send and receive
  reporting the same message size. Uncheck this for Scalasca-generated OTF2.
* Idealized order of receives: Change the order of receives from their true
  physical time order to an idealized one in each phase. We recommend this for
  Charm++. It is not compatible with clustering.
* Advanced stepping within a partition: Align sends based on happened-before
  structure rather than as early as possible. MPI only.
* Coalesce Isends: Groups neighboring `MPI_Isend`s into a single operation
  which may send to multiple receive operations. We recommend this option as a
  default for all MPI traces.
* Cluster processes: Shows a cluster view that clusters the processes by the
  active metric. This is useful for large process counts. MPI only.
  * Seed: Set seed for repeatable clustering.

### Navigating Traces

The three timeline views support linked panning, zooming and selection. The
overview shows the total metric value over time steps for the whole trace.
Clicking and dragging in this view will select a span of timesteps in the
other views.

Navigation | Control
-----------|---------
Pan | Left-click drag
Zoom in time | Mouse wheel
Zoom in processes | Shift + Mouse wheel
Zoom to rectangle | Right-click drag rectangle
Select operation | Right-click operation
Tool tips | Hover

The cluster view has a slider which changes the size of the neighborhood shown
in the upper part of the view. The lower part of the view shows the clusters.
Left-click to divide clusters into its children. Click on dendrogram nodes to
collapse clusters. Dendrogram pertains to left-most visible partition.
Clustering currently shows the first partition rather than all.

### Saving Traces
All traces are saved in OTF2 and include only the information from the
original trace that is used by Ravel. In addition, communication-related
operations used for logical structure have an `OTF2_AttributeList` associated
with their Leave events. These lists include a `phase` and `step` value
defining the logical structure used by Ravel, as well as any metric values
computed for that operation. Any metric values ending in `_agg` represent the
calculated value of the aggregated non-communication operation directly
preceding.


Authors
-------
Ravel was written by Kate Isaacs.

License
-------
Ravel is released under the LGPL license. For more details see the LICENSE
file.

LLNL-CODE-663885

Related Publications
--------------------
Katherine E. Isaacs, Peer-Timo Bremer, Ilir Jusufi, Todd Gamblin, Abhinav
Bhatele, Martin Schulz, and Bernd Hamann. Combing the Communication Hairball:
Visualizing Parallel Execution Traces using Logical Time. *IEEE Transactions on
Visualization and Computer Graphics, Proceedings of InfoVis '14*, 20(12):2349-2358, December 2014. 
[DOI: 10.1109/TVCG.2014.2346456](http://dx.doi.org/10.1109/TVCG.2014.2346456)

Katherine E. Isaacs, Abhinav Bhatele, Jonathan Lifflander, David Boehme, Todd
Gamblin, Bernd Hamann, Peer-Timo Bremer. Recovering Logical Structure from
Charm++ Event Traces. In *Proceedings fo the ACM/IEEE Conference on
Supercomputing (SC15)*, November 2015. [DOI:
10.1145/2807591.2807634](http://dx.doi.org/10.1145/2807591.2807634)

Katherine E. Isaacs, Todd Gamblin, Abhinav Bhatele, Martin Schulz, Bernd
Hamann, and Peer-Timo Bremer. Ordering Traces Logically to Identify Lateness
in Message Passing Programs. *IEEE Transactions on Parallel and Distributed
Systems*, 27(3):829-840, March 2016. [DOI:
10.1109/TPDS.2015.2417531](http://dx.doi.org/10.1109/TPDS.2015.2417531)
