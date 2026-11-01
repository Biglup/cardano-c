Deep copying native objects
===========================

Use ``cardano_<type>_deep_clone`` to obtain an independent, owned copy of a
native value. Release the copy with the corresponding ``*_unref`` function.

See the `deep-copy API guide <https://github.com/Biglup/cardano-c/blob/main/DEEP_CLONE.md>`_
for ownership rules, supported types, and testing guidance. Individual type
reference pages document each function.
