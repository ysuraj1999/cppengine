(in-package :playground)

(defmacro with-renderer (&body body)
  (mu:with-gensyms (ok)
    `(progn
       (setf ,ok (%renderer-init))
       (unwind-protect
            (when ,ok ,@body)
         (when ,ok
           (%renderer-quit))))))

(defmacro do-scenes ((scene) &body body)
  (mu:with-gensyms (handle)
    `(map nil (lambda (,handle)
                (let ((,scene (ensure-scene ,handle)))
                  ,@body))
          (scene-handles))))

(defmacro do-scene-meshes ((mesh scene) &body body)
  `(map nil (lambda (,mesh) ,@body) (scene-meshes ,scene)))

(defmacro do-mesh-instances ((instance mesh) &body body)
  `(map nil (lambda (,instance) ,@body) (mesh-instances ,mesh)))

(defmacro do-scene-instances ((instance scene) &body body)
  (mu:with-gensyms (mesh)
    `(do-scene-meshes (,mesh ,scene)
       (do-mesh-instances (,instance ,mesh)
         ,@body))))

(defmacro do-input-events ((device event) &body body)
  "Unified discrete event iteration. DEVICE is :mouse or :keyboard.
   Iterates the existing %input-*-event-count/at buffers - no
   duplicated buffering or processing."
  (mu:with-gensyms (i)
    (ecase device
      (:mouse
       `(dotimes (,i (%input-mouse-event-count))
          (let ((,event (%input-mouse-event-at ,i)))
            ,@body)))
      (:keyboard
       `(dotimes (,i (%input-keyboard-event-count))
          (let ((,event (%input-keyboard-event-at ,i)))
            ,@body))))))

(defmacro input (device &rest args)
  "Direct state query - expands at compile time to the existing %input-*
   native functions. No runtime abstraction.
     (input :mouse :position :x)         -> (%input-mouse-x)
     (input :mouse :delta :x)            -> (%input-mouse-dx)
     (input :mouse :wheel :y)            -> (%input-mouse-wheel-y)
     (input :mouse :button :left :down)  -> (%input-mouse-down-p :left)
     (input :keyboard :w :down)          -> (%input-key-down-p (key :w))
     (input :cursor :captured-p)         -> (%input-cursor-captured-p)
     (input :cursor :capture t)          -> (%input-set-cursor-captured t)"
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
          `(%input-cursor-captured-p))
         (:capture
          (destructuring-bind (flag) rest
            `(%input-set-cursor-captured ,flag))))))
    (:keyboard
     (destructuring-bind (keyname action) args
       (unless (mouse-action-p action)
         (error "Key action isn't valid: ~S" action))
       `(,(key-action-to-function action) (key ,keyname))))
    (:cursor
     (destructuring-bind (property &rest rest) args
       (ecase property
         (:captured-p
          `(%input-cursor-captured-p))
         (:capture
          (destructuring-bind (flag) rest
            `(%input-set-cursor-captured ,flag))))))))

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
    (:down (quote %input-mouse-down-p))
    (:up (quote %input-mouse-up-p))
    (:pressed (quote %input-mouse-pressed-p))
    (:released (quote %input-mouse-released-p))))

(defun axis-to-position-function (axis)
  (ecase axis
    (:x (quote %input-mouse-x))
    (:y (quote %input-mouse-y))))

(defun axis-to-delta-function (axis)
  (ecase axis
    (:x (quote %input-mouse-dx))
    (:y (quote %input-mouse-dy))))

(defun axis-to-wheel-function (axis)
  (ecase axis
    (:x (quote %input-mouse-wheel-x))
    (:y (quote %input-mouse-wheel-y))))

(defmacro with-mouse-state (bindings &body body)
  (let ((mouse-bindings))
    (dolist (binding bindings)
      (destructuring-bind (type &rest args) binding
        (case type
          (:button
           (destructuring-bind (button action name) args
             (unless (mouse-button-p button)
               (error "Button isn't valid: ~S" button))
             (unless (mouse-action-p action)
               (error "Button action isn't valid: ~S" action))
             (push `(,name (funcall ',(button-action-to-function action) ,button)) mouse-bindings)))
          (:position
           (destructuring-bind (axis name) args
             (unless (mouse-position-property-p axis)
               (error "Axis not valid ~S" axis))
             (push `(,name (funcall ',(axis-to-position-function axis))) mouse-bindings)))
          (:wheel
           (destructuring-bind (axis name) args
             (unless (mouse-wheel-property-p axis)
               (error "Axis not valid ~S" axis))
             (push `(,name (funcall ',(axis-to-wheel-function axis))) mouse-bindings)))
          (:delta
           (destructuring-bind (axis name) args
             (unless (mouse-delta-property-p axis)
               (error "Axis not valid ~S" axis))
             (push `(,name (funcall ',(axis-to-delta-function axis))) mouse-bindings)))
          (otherwise (error "Not a valid mouse binding: ~S" binding)))))
    `(mu:with-let ,(nreverse mouse-bindings) ,@body)))

(defun key-action-to-function (action)
  (ecase action
    (:down (quote %input-key-down-p))
    (:up (quote %input-key-up-p))
    (:pressed (quote %input-key-pressed-p))
    (:released (quote %input-key-released-p))))

(defmacro with-keyboard-state (bindings &body body)
  (let ((key-bindings))
    (dolist (binding bindings)
      (destructuring-bind (keyname action name) binding
        (unless (mouse-action-p action)
          (error "Key action isn't valid: ~S" action))
        (push `(,name (funcall ',(key-action-to-function action) (key ,keyname)))
              key-bindings)))
    `(mu:with-let ,(nreverse key-bindings) ,@body)))
