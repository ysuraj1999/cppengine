(asdf:defsystem "playground"
  :description "Renderer"
  :author "Suraj Yadav"
  :license "MIT"
  :serial t

  :depends-on
  (:cffi
   :cffi-libffi
   :lparallel
   :alexandria)

  :components
  ((:file "package")
   (:file "playground")))
