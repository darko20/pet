(lkb-load-lisp (this-directory) "globals.lsp")
(lkb-load-lisp (this-directory) "user-fns.lsp")
(load-lkb-preferences (this-directory) "user-prefs.lsp")

(read-tdl-type-files-aux
     (list (lkb-pathname (parent-directory) "types.tdl")))
(read-tdl-lex-file-aux 
     (lkb-pathname (parent-directory) "lexicon.tdl"))
(read-morph-file-aux 
     (lkb-pathname (parent-directory) "inflr.tdl"))

(batch-check-lexicon)

(read-tdl-grammar-file-aux 
     (lkb-pathname (parent-directory) "rules.tdl"))
(read-tdl-lex-rule-file-aux 
     (lkb-pathname (parent-directory) "lrules.tdl"))
(read-tdl-start-file-aux 
     (lkb-pathname (parent-directory) "start.tdl"))
(read-tdl-parse-node-file-aux 
     (lkb-pathname (parent-directory) "parse-nodes.tdl"))



