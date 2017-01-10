TODO
====

* **Non-monotone partitioning edge cases**

	Non-monotone partitioning fails sometimes by drawing diagonals the are not
	contained within the polygon. One example is with the polygon
	`{0, 0}, {0, 72}, {24, 72}, {72, 48}, {24, 24}, {48, 12}, {120, 48}, {72, 72}, {144, 72}, {144, 0}`.
	The partitions of the polygon overlap each other and the triangulation of these
	partitions creates far too many triangles. The rendered polygon is also
	incorrect.
