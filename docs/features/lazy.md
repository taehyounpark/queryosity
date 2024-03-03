All actions in a dataflow are *lazy*, meaning they are not executed until and unless needed: 

1. An query is executed only if its associated selection passes its cut.
2. A cut will be evaluated only if all upstream selections in its chain have passed.
3. A column will be evaluated only if it is needed to determine any of the above.