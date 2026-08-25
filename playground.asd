(asdf:defsystem "playground"
  :description "Renderer"
  :author "Suraj Yadav"
  :license "MIT"
  :serial t

  :depends-on
  (:cffi
   :cffi-libffi
   :lparallel
   :alexandria
   :my-utilities)

  :components
  ((:file "package")
   (:file "library")
   (:file "macros")
   (:file "playground")))
