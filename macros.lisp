(in-package :playground)

(defmacro with-renderer (&body body)
  (mu:with-gensyms (ok)
    `(progn
       (setf ,ok (c-renderer-init))
       (unwind-protect
            (when ,ok ,@body)
         (when ,ok
           (c-renderer-quit))))))

(defmacro with-ui-window (windowstr &body body)
  "Begin a UI window titled WINDOWSTR, run BODY, and end the window.
   The end runs even on a non-local exit."
  `(progn
     (c-ui-begin-window ,windowstr)
     (unwind-protect
          (progn ,@body)
       (c-ui-end-window))))

(defmacro with-ui-id (id &body body)
  "Push ID onto the UI id stack, run BODY, and pop it.  The pop runs
   even on a non-local exit."
  `(progn
     (c-ui-push-id ,id)
     (unwind-protect
          (progn ,@body)
       (c-ui-pop-id))))

(defmacro with-ui-tree (nodestring &optional open-sym &body body)
  "Submit a tree node labeled NODESTRING and run BODY.  The node's UI id
   is NODESTRING itself.  BODY runs every frame so click and context
   handlers stay attached even while the node is closed; guard children
   with OPEN-SYM, bound to whether the node is open.  The tree closes
   every frame the node is open."
  (mu:with-gensyms (open)
    (let ((open-sym (or open-sym open)))
      `(let ((,open-sym (c-ui-begin-tree ,nodestring)))
         (unwind-protect
              (progn ,@body)
           (when ,open-sym
             (c-ui-end-tree)))))))

(defmacro do-meshes ((mesh) &body body)
  "Traverse every mesh in the renderer's single scene. MESH is bound to a
mesh handle, a first-class value that may be saved or passed to functions.
Iterates via the count/at C API without building an intermediate vector."
  (mu:with-gensyms (i)
    `(dotimes (,i (c-mesh-count))
       (let ((,mesh (c-mesh-at ,i)))
         ,@body))))

(defmacro do-instances ((instance mesh) &body body)
  "Traverse every instance of MESH (a mesh handle) in the flat instance
array. INSTANCE is bound to the flat instance index, a first-class value
that may be saved or passed to functions."
  (mu:with-gensyms (i)
    `(dotimes (,i (c-instance-count))
       (when (eql (c-instance-mesh ,i) ,mesh)
         (let ((,instance ,i))
           ,@body)))))

(defmacro do-input-events ((event) &body body)
  "Iterate the unified input event stream delivered this frame. BODY runs
   once per event with EVENT bound to an INPUT-EVENT carrying TYPE, KEY,
   BUTTON, DOWN, REPEAT and the mouse fields."
  (mu:with-gensyms (i)
    `(dotimes (,i (c-input-event-count))
       (let ((,event (c-input-event-at ,i)))
         ,@body))))

(defmacro input (device &rest args)
  "Direct state query - expands at compile time to the existing c-input-*
   native functions. No runtime abstraction.
     (input :mouse :position :x)         -> (c-input-mouse-x)
     (input :mouse :delta :x)            -> (c-input-mouse-dx)
     (input :mouse :wheel :y)            -> (c-input-mouse-wheel-y)
     (input :mouse :button :left :down)  -> (c-input-mouse-down-p :left)
       (input :keyboard :w :down)          -> (c-input-key-down-p :w)
     (input :cursor :captured-p)         -> (c-input-cursor-captured-p)
     (input :cursor :capture t)          -> (c-input-set-cursor-captured t)"
  (ecase device
    (:mouse
     (destructuring-bind (property &rest rest) args
       (ecase property
         (:position
          (destructuring-bind (axis) rest
            (unless (mouse-position-property-p axis)
              (error "Axis not valid ~S" axis))
            `(,(axis-to-position-function axis))))
         (:delta
          (destructuring-bind (axis) rest
            (unless (mouse-delta-property-p axis)
              (error "Axis not valid ~S" axis))
            `(,(axis-to-delta-function axis))))
         (:wheel
          (destructuring-bind (axis) rest
            (unless (mouse-wheel-property-p axis)
              (error "Axis not valid ~S" axis))
            `(,(axis-to-wheel-function axis))))
         (:button
          (destructuring-bind (button action) rest
            (unless (mouse-button-p button)
              (error "Button isn't valid: ~S" button))
            (unless (mouse-action-p action)
              (error "Button action isn't valid: ~S" action))
            `(,(button-action-to-function action) ,button)))
         (:captured-p
          `(c-input-cursor-captured-p))
         (:capture
          (destructuring-bind (flag) rest
            `(c-input-set-cursor-captured ,flag))))))
    (:keyboard
     (destructuring-bind (keyname action) args
       (unless (mouse-action-p action)
         (error "Key action isn't valid: ~S" action))
        `(,(key-action-to-function action) ,keyname)))
    (:cursor
     (destructuring-bind (property &rest rest) args
       (ecase property
         (:captured-p
          `(c-input-cursor-captured-p))
         (:capture
          (destructuring-bind (flag) rest
            `(c-input-set-cursor-captured ,flag))))))))

(defun mouse-button-p (pred)
  (member pred '(:left :middle :right :x1 :x2)))

(defun mouse-action-p (pred)
  (member pred '(:down :up :pressed :released)))

(defun mouse-axis-p (pred)
  (member pred '(:x :y)))

(defun mouse-position-property-p (property)
  (mouse-axis-p property))

(defun mouse-delta-property-p (property)
  (mouse-axis-p property))

(defun mouse-wheel-property-p (property)
  (mouse-axis-p property))

(defun button-action-to-function (action)
  (ecase action
    (:down (quote c-input-mouse-down-p))
    (:up (quote c-input-mouse-up-p))
    (:pressed (quote c-input-mouse-pressed-p))
    (:released (quote c-input-mouse-released-p))))

(defun axis-to-position-function (axis)
  (ecase axis
    (:x (quote c-input-mouse-x))
    (:y (quote c-input-mouse-y))))

(defun axis-to-delta-function (axis)
  (ecase axis
    (:x (quote c-input-mouse-dx))
    (:y (quote c-input-mouse-dy))))

(defun axis-to-wheel-function (axis)
  (ecase axis
    (:x (quote c-input-mouse-wheel-x))
    (:y (quote c-input-mouse-wheel-y))))

(defun key-action-to-function (action)
  (ecase action
    (:down (quote c-input-key-down-p))
    (:up (quote c-input-key-up-p))
    (:pressed (quote c-input-key-pressed-p))
    (:released (quote c-input-key-released-p))))

(defmacro edit-transform ((transform) &body clauses)
  "Build a fresh transform by applying clause edits and return it.
   A default transform with zero position, unit scale and zero rotation
   is created, then each clause is applied in order as one atomic edit:

     (:position :x 10)    set position
     (:move-by :z 5)      add to position

     (:rotation :y 90)    set rotation
     (:rotate-by :x 15)   add to rotation

     (:scale :x 2)        set scale
     (:scale-by :y 1.5)   multiply scale

   All clauses run against the same transform object, which is the value
   of the entire form."
  (labels ((axis-accessor (prefix axis)
             (ecase prefix
               (:p (ecase axis
                     (:x 'transform-px) (:y 'transform-py) (:z 'transform-pz)))
               (:r (ecase axis
                     (:x 'transform-rx) (:y 'transform-ry) (:z 'transform-rz)))
               (:s (ecase axis
                     (:x 'transform-sx) (:y 'transform-sy) (:z 'transform-sz)))))
           (op-prefix (op)
             (ecase op
               ((:position :move-by) :p)
               ((:rotation :rotate-by) :r)
               ((:scale :scale-by) :s)))
           (assign (accessor value)
             `(setf (,accessor ,transform) ,value))
           (relative (accessor value combine)
             `(setf (,accessor ,transform)
                    (,combine (,accessor ,transform) ,value)))
           (clause-form (clause)
             (unless (= 3 (length clause))
               (error "Invalid transform clause ~S: expected exactly (operation axis value)"
                      clause))
             (destructuring-bind (op axis value) clause
               (unless (member axis '(:x :y :z))
                 (error "Invalid transform axis ~S in clause ~S: expected :X :Y or :Z"
                        axis clause))
               (let ((accessor (axis-accessor (op-prefix op) axis)))
                 (ecase op
                   ((:position :rotation :scale) (assign accessor value))
                   ((:move-by :rotate-by) (relative accessor value '+))
                   ((:scale-by) (relative accessor value '*)))))))
    `(progn
       ,@(mapcar #'clause-form clauses)
       ,transform)))
