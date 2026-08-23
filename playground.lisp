(in-package :playground)

(define-foreign-library libcppengine
    (:unix (:default "./build/libcppengine"))
    (t (:default "./build/libcppengine")))

(unless (foreign-library-loaded-p 'libcppengine)
    (use-foreign-library libcppengine))

(defctype %scene-handle :uint64)
(defctype %mesh-handle :uint64)
(defctype %instance-handle :uint64)

(defcstruct (%transform :class cffi-transform)
  (px :float) (py :float) (pz :float)
  (sx :float) (sy :float) (sz :float)
  (rx :float) (ry :float) (rz :float))

(defun key (name)
  (ecase name
    ;; Letters
    (:a 4) (:b 5) (:c 6) (:d 7) (:e 8) (:f 9) (:g 10) (:h 11)
    (:i 12) (:j 13) (:k 14) (:l 15) (:m 16) (:n 17) (:o 18) (:p 19)
    (:q 20) (:r 21) (:s 22) (:t 23) (:u 24) (:v 25) (:w 26) (:x 27)
    (:y 28) (:z 29)
    ;; Numbers
    (:1 30) (:2 31) (:3 32) (:4 33) (:5 34)
    (:6 35) (:7 36) (:8 37) (:9 38) (:0 39)
    ;; Common
    (:enter 40) (:escape 41) (:backspace 42) (:tab 43) (:space 44)
    ;; Punctuation
    (:minus 45) (:equals 46) (:left-bracket 47) (:right-bracket 48)
    (:backslash 49) (:semicolon 51) (:apostrophe 52) (:grave 53)
    (:comma 54) (:period 55) (:slash 56)
    ;; Function keys
    (:f1 58) (:f2 59) (:f3 60) (:f4 61) (:f5 62) (:f6 63)
    (:f7 64) (:f8 65) (:f9 66) (:f10 67) (:f11 68) (:f12 69)
    ;; Navigation
    (:insert 73) (:home 74) (:page-up 75) (:delete 76) (:end 77)
    (:page-down 78) (:right 79) (:left 80) (:down 81) (:up 82)
    ;; Modifiers
    (:left-ctrl 224) (:left-shift 225) (:left-alt 226) (:left-gui 227)
    (:right-ctrl 228) (:right-shift 229) (:right-alt 230) (:right-gui 231)))

(defcfun ("renderer_init" %renderer-init) :bool)

(defcfun ("scene_create" %scene-create) %scene-handle)
(defcfun ("scene_destroy" %scene-destroy) :bool
  (sh %scene-handle))

(defcfun ("scene_load_obj" %scene-load-obj) :bool
  (sh %scene-handle)
  (path :string))

(defcfun ("scene_remove_mesh" %scene-remove-mesh) :bool
  (sh %scene-handle)
  (mh %mesh-handle))

(defcfun ("scene_add_instance" %scene-add-instance) %instance-handle
  (sh %scene-handle)
  (m %mesh-handle)
  (xform (:struct %transform)))

;; Returns a pointer into engine-owned memory; do not free.
(defcfun ("scene_mesh_alias" %scene-mesh-alias) :pointer
  (sh %scene-handle)
  (mh %mesh-handle))

(defun scene-mesh-alias (sh mh)
  (foreign-string-to-lisp (%scene-mesh-alias sh mh)))

;; Returns a pointer into engine-owned memory; do not free.
(defcfun ("scene_instance_alias" %scene-instance-alias) :pointer
  (sh %scene-handle)
  (i %instance-handle))

(defun scene-instance-alias (sh i)
  (foreign-string-to-lisp (%scene-instance-alias sh i)))

(defcfun ("scene_instance_get_transform" %scene-instance-get-transform)
    (:struct %transform)
  (sh %scene-handle)
  (i %instance-handle))

(defcfun ("scene_instance_set_transform"
          %scene-instance-set-transform) :void
  (sh :uint64)
  (i :uint64)
  (xform (:struct %transform)))

(defcfun ("scene_remove_instance" %scene-remove-instance) :bool
  (sh %scene-handle)
  (i %instance-handle))

(defcfun ("renderer_sync_scene" %renderer-sync-scene) :bool
  (sh %scene-handle))
(defcfun ("renderer_render_begin" %renderer-render-begin) :void)
(defcfun ("renderer_render_scene" %renderer-render-scene) :void
  (sh %scene-handle))
(defcfun ("renderer_render_end" %renderer-render-end) :void)

(defcfun ("ui_begin" %ui-begin) :void)
(defcfun ("ui_end" %ui-end) :void)

(defcfun ("ui_begin_window" %ui-begin-window)
  :void
  (name :string))
(defcfun ("ui_end_window" %ui-end-window)
  :void)
(defcfun ("ui_button" %ui-button)
  :bool
  (label :string))

(defcfun ("ui_treenode" %ui-treenode)
  :bool
  (label :string))

(defcfun ("ui_treepop" %ui-treepop)
  :void)

(defcfun ("ui_selectable" %ui-selectable)
  :bool
  (label :string))

(defcfun ("ui_sameline" %ui-sameline)
  :void)

(defcfun ("ui_drag_float3" %ui-drag-float3)
  :bool
  (label :string)
  (values :pointer))

(defcfun ("ui_text" %ui-text)
  :void
  (text :string))

(defcfun ("renderer_clear" %renderer-clear) :bool)

(defcfun ("renderer_quit" %renderer-quit) :void)

(defcfun ("input_poll" %input-poll) :void)

(defcfun ("input_should_close" %input-should-close-p) :bool)

(defcfun ("scene_count" %scene-count) :uint64)

(defcfun ("scene_at" %scene-at) %scene-handle
  (index :uint64))

(defcfun ("scene_mesh_count" %scene-mesh-count) :uint64
  (sh %scene-handle))

(defcfun ("scene_mesh_at" %scene-mesh-at) %mesh-handle
  (sh %scene-handle)
  (index :uint64))

(defcfun ("scene_mesh_instance_count" %scene-mesh-instance-count) :uint64
  (sh %scene-handle)
  (mesh %mesh-handle))

(defcfun ("scene_mesh_instance_at" %scene-mesh-instance-at) %instance-handle
  (sh %scene-handle)
  (mesh %mesh-handle)
  (index :uint64))

(defcfun ("renderer_camera_set" %renderer-camera-set) :void
  (px :float) (py :float) (pz :float)
  (yaw :float) (pitch :float))

(defcenum (%mouse-button :int)
  (:left 1)
  (:middle 2)
  (:right 3)
  (:x1 4)
  (:x2 5))

(defcfun ("input_mouse_x" %input-mouse-x) :float)
(defcfun ("input_mouse_y" %input-mouse-y) :float)
(defcfun ("input_mouse_dx" %input-mouse-dx) :float)
(defcfun ("input_mouse_dy" %input-mouse-dy) :float)
(defcfun ("input_mouse_wheel_x" %input-mouse-wheel-x) :float)
(defcfun ("input_mouse_wheel_y" %input-mouse-wheel-y) :float)
(defcfun ("input_mouse_down" %input-mouse-down-p) :bool
  (button %mouse-button))
(defcfun ("input_mouse_pressed" %input-mouse-pressed-p) :bool
  (button %mouse-button))
(defcfun ("input_mouse_released" %input-mouse-released-p) :bool
  (button %mouse-button))

(defcfun ("renderer_camera_move" %renderer-camera-move) :void
  (forward :float)
  (right :float)
  (up :float))

(defcfun ("renderer_camera_rotate" %renderer-camera-rotate) :void
  (yaw :float)
  (pitch :float))

(defcfun ("renderer_camera_update" %renderer-camera-update) :void)

(defcfun ("input_key_down" %input-key-down-p) :bool
  (key :int))

(defcfun ("input_key_pressed" %input-key-pressed-p) :bool
  (key :int))

(defcfun ("input_key_released" %input-key-released-p) :bool
  (key :int))

(defcstruct (%keyboard-event :class cffi-keyboard-event)
  (scancode :int)
  (down :bool)
  (repeat :bool))

(defcfun ("input_keyboard_event_count" %input-keyboard-event-count) :uint64)

(defcfun ("input_keyboard_event_at" %input-keyboard-event-at)
    (:struct %keyboard-event)
  (index :uint64))



(defcfun ("time_dt" %time-dt) :float)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defstruct keyboard-event
  (scancode 0 :type (signed-byte 32))
  (down nil :type boolean)
  (repeat nil :type boolean))

(defun keyboard-events ()
  "Return a typed vector of this frame's key events in the order SDL
delivered them; each element is a KEYBOARD-EVENT with SCANCODE, DOWN and
REPEAT."
  (let ((count (%input-keyboard-event-count)))
    (loop with events = (make-array count
                                    :element-type 'keyboard-event
                                    :fill-pointer 0)
          for i below count
          for e = (%input-keyboard-event-at i)
          do (vector-push-extend
              (make-keyboard-event
               :scancode (slot-value e 'scancode)
               :down (slot-value e 'down)
               :repeat (slot-value e 'repeat))
              events)
          finally (return events))))

(deftype u64 () '(unsigned-byte 64))

(defstruct transform
  (px 0.0 :type single-float)
  (py 0.0 :type single-float)
  (pz 0.0 :type single-float)
  (sx 1.0 :type single-float)
  (sy 1.0 :type single-float)
  (sz 1.0 :type single-float)
  (rx 0.0 :type single-float)
  (ry 0.0 :type single-float)
  (rz 0.0 :type single-float))

(defun transform! (px py pz sx sy sz rx ry rz)
  (make-transform
   :px px :py py :pz pz
   :sx sx :sy sy :sz sz
   :rx rx :ry ry :rz rz))

(defmethod translate-into-foreign-memory ((value transform)
                                          (type cffi-transform)
                                          ptr)
  (setf (foreign-slot-value ptr '(:struct %transform) 'px) (transform-px value)
        (foreign-slot-value ptr '(:struct %transform) 'py) (transform-py value)
        (foreign-slot-value ptr '(:struct %transform) 'pz) (transform-pz value)
        (foreign-slot-value ptr '(:struct %transform) 'sx) (transform-sx value)
        (foreign-slot-value ptr '(:struct %transform) 'sy) (transform-sy value)
        (foreign-slot-value ptr '(:struct %transform) 'sz) (transform-sz value)
        (foreign-slot-value ptr '(:struct %transform) 'rx) (transform-rx value)
        (foreign-slot-value ptr '(:struct %transform) 'ry) (transform-ry value)
        (foreign-slot-value ptr '(:struct %transform) 'rz) (transform-rz value)))

(defmethod translate-from-foreign (ptr (type cffi-transform))
  (transform! (foreign-slot-value ptr '(:struct %transform) 'px)
              (foreign-slot-value ptr '(:struct %transform) 'py)
              (foreign-slot-value ptr '(:struct %transform) 'pz)
              (foreign-slot-value ptr '(:struct %transform) 'sx)
              (foreign-slot-value ptr '(:struct %transform) 'sy)
              (foreign-slot-value ptr '(:struct %transform) 'sz)
              (foreign-slot-value ptr '(:struct %transform) 'rx)
              (foreign-slot-value ptr '(:struct %transform) 'ry)
              (foreign-slot-value ptr '(:struct %transform) 'rz)))


(defvar *mesh-objects* (make-hash-table :test #'eql))
(defvar *instance-objects* (make-hash-table :test #'eql))

(defun ensure-mesh (scene handle)
  (or (gethash handle *mesh-objects*)
      (setf (gethash handle *mesh-objects*)
            (make-instance 'mesh :handle handle :scene scene))))

(defun ensure-instance (mesh handle)
  (or (gethash handle *instance-objects*)
      (setf (gethash handle *instance-objects*)
            (make-instance 'instance :handle handle :mesh mesh))))

(defclass instance ()
  ((handle :reader instance-handle :type u64 :initarg :handle)
   (mesh :reader instance-mesh :initarg :mesh)))

(defmethod print-object ((instance instance) stream)
  (print-unreadable-object (instance stream :type t :identity t)
    (format stream
            "handle=~a mesh=~a"
            (instance-handle instance)
            (mesh-handle (instance-mesh instance)))))

(defmethod instance-transform ((instance instance))
  (%scene-instance-get-transform
   (scene-handle (mesh-scene (instance-mesh instance)))
   (instance-handle instance)))

(defmethod (setf instance-transform) (value (instance instance))
  (%scene-instance-set-transform
   (scene-handle (mesh-scene (instance-mesh instance)))
   (instance-handle instance)
   value)
  value)

(defmethod instance-alias ((instance instance))
  (foreign-string-to-lisp
   (%scene-instance-alias
    (scene-handle (mesh-scene (instance-mesh instance)))
    (instance-handle instance))))

(defclass mesh ()
  ((handle :reader mesh-handle :type u64 :initarg :handle)
   (scene :reader mesh-scene :initarg :scene)))

(defmethod print-object ((mesh mesh) stream)
  (print-unreadable-object (mesh stream :type t :identity t)
    (format stream
            "handle=~a instances=~a"
            (mesh-handle mesh)
            (%scene-mesh-instance-count
             (scene-handle (mesh-scene mesh))
             (mesh-handle mesh)))))

(defmethod mesh-alias ((mesh mesh))
  (foreign-string-to-lisp
   (%scene-mesh-alias
    (scene-handle (mesh-scene mesh))
    (mesh-handle mesh))))

(defmethod mesh-instances ((mesh mesh))
  (let ((sh (scene-handle (mesh-scene mesh)))
        (mh (mesh-handle mesh)))
    (loop with count = (%scene-mesh-instance-count sh mh)
          with instances = (make-array count :fill-pointer 0)
          for i below count
          do (vector-push-extend
              (ensure-instance mesh (%scene-mesh-instance-at sh mh i))
              instances)
          finally (return instances))))

(defmethod mesh-add-instance ((mesh mesh) (transform transform))
  (ensure-instance
   mesh
   (%scene-add-instance
    (scene-handle (mesh-scene mesh))
    (mesh-handle mesh)
    transform)))

(defclass scene ()
  ((handle :reader scene-handle :type u64)))

(defmethod print-object ((scene scene) stream)
  (print-unreadable-object (scene stream :type t :identity t)
    (format stream
            "handle=~a meshes=~a instances=~a"
            (scene-handle scene)
            (%scene-mesh-count (scene-handle scene))
            (loop for mesh across (scene-meshes scene)
                  sum (%scene-mesh-instance-count
                       (scene-handle scene)
                       (mesh-handle mesh))))))

(defmethod initialize-instance :after ((scene scene) &rest initargs)
  (declare (ignore initargs))
  (setf (slot-value scene 'handle) (%scene-create)))

(defmethod scene-load-obj ((scene scene) path)
  (%scene-load-obj (scene-handle scene) path))

(defmethod scene-meshes ((scene scene))
  (let ((sh (scene-handle scene)))
    (loop with count = (%scene-mesh-count sh)
          with meshes = (make-array count :fill-pointer 0)
          for i below count
          do (vector-push-extend (ensure-mesh scene (%scene-mesh-at sh i)) meshes)
          finally (return meshes))))

(defmethod scene-sync ((scene scene))
  (%renderer-sync-scene (scene-handle scene)))

(defmethod scene-render ((scene scene))
  (%renderer-render-scene (scene-handle scene)))

(defun drop-scene-proxies (scene)
  (let ((dead-meshes ())
        (dead-instances ()))
    (maphash (lambda (handle mesh)
               (declare (ignore handle))
               (when (eq (mesh-scene mesh) scene)
                 (push mesh dead-meshes)))
             *mesh-objects*)
    (dolist (mesh dead-meshes)
      (remhash (mesh-handle mesh) *mesh-objects*))
    (maphash (lambda (handle instance)
               (declare (ignore handle))
               (when (eq (mesh-scene (instance-mesh instance)) scene)
                 (push instance dead-instances)))
             *instance-objects*)
    (dolist (instance dead-instances)
      (remhash (instance-handle instance) *instance-objects*))))

(defmethod scene-destroy ((scene scene))
  (prog1 (%scene-destroy (scene-handle scene))
    (drop-scene-proxies scene)))

(defmethod scene-remove-mesh ((scene scene) (mesh mesh))
  (let ((ok (%scene-remove-mesh (scene-handle scene) (mesh-handle mesh))))
    (when ok
      (let ((dead ()))
        (remhash (mesh-handle mesh) *mesh-objects*)
        (maphash (lambda (handle instance)
                   (declare (ignore handle))
                   (when (eq (instance-mesh instance) mesh)
                     (push instance dead)))
                 *instance-objects*)
        (dolist (instance dead)
          (remhash (instance-handle instance) *instance-objects*))))
    ok))

(defmethod scene-remove-instance ((scene scene) (instance instance))
  (let ((ok (%scene-remove-instance (scene-handle scene) (instance-handle instance))))
    (when ok
      (remhash (instance-handle instance) *instance-objects*))
    ok))

(defun clamp(val minimum maximum)
  (min (max val minimum) maximum))

(defun wrap(value maximum)
  (if (>= value maximum)
      0.0
      value))

(defparameter *keyboard-sensitivity* 200.0)
(defparameter *mouse-sensitivity* 50.0)

;; (require :sb-sprof)
;; (defmacro sbcl-profile(&body body)
;;   `(sb-sprof:with-profiling (:report :flat)
;;      ,@body))

(defvar *scene* nil)
(defvar *selection* nil)

(defun instance-properties-panel ()
  (when *selection*
    (%ui-begin-window "Instance Properties")
    (let* ((sel *selection*)
           (tr (%scene-instance-get-transform
                (scene-handle (mesh-scene (instance-mesh sel)))
                (instance-handle sel))))
      (%ui-text (format nil "Position: ~,2f ~,2f ~,2f"
                       (slot-value tr 'px) (slot-value tr 'py) (slot-value tr 'pz)))
      (%ui-text (format nil "Rotation: ~,2f ~,2f ~,2f"
                       (slot-value tr 'rx) (slot-value tr 'ry) (slot-value tr 'rz)))
      (%ui-text (format nil "Scale: ~,2f ~,2f ~,2f"
                       (slot-value tr 'sx) (slot-value tr 'sy) (slot-value tr 'sz))))
    (%ui-end-window)))

(defun demo ()
  (%renderer-init)
  (let ((scene (make-instance 'scene)))
    (setf *scene* scene)
    (scene-load-obj scene "models/bmw.obj")
    (loop for mesh across (scene-meshes scene)
	  doing
	     (mesh-add-instance mesh (transform! 10.0 0.0 0.0 1.0 1.0 1.0 0.0 0.0 0.0)))
    (scene-sync scene)
    (loop until (%input-should-close-p)
	  for dt = (%time-dt)
	  doing
	     (%input-poll)
	     (let* ((wheel-y (%input-mouse-wheel-y))
		    (wheel-x (%input-mouse-wheel-x))
		    (dx (%input-mouse-dx))
		    (dy (%input-mouse-dy))
		    (w-down (%input-key-down-p (key :w)))
		    (a-down (%input-key-down-p (key :a)))
		    (s-down (%input-key-down-p (key :s)))
		    (d-down (%input-key-down-p (key :d)))
		    (space-down (%input-key-down-p (key :space)))
		    (ctrl-down (%input-key-down-p (key :left-ctrl)))
		    (move-speed (* *keyboard-sensitivity* dt))
		    (look-speed (* *mouse-sensitivity* dt))
		    (middle-click (%input-mouse-down-p :middle)))
	       (%renderer-camera-rotate (* (- wheel-x) look-speed) (* wheel-y look-speed))
	       (%renderer-camera-move
		(+ (if w-down move-speed 0.0)
		   (if s-down (- move-speed) 0.0))
		(if middle-click
		    (* (- dx) look-speed)
		    0.0)
		(if middle-click
		    (* dy look-speed)
		    0.0)))
	     (%renderer-camera-update)
	     (%renderer-render-begin)
	     (%renderer-render-scene (scene-handle scene))
	     (%ui-begin)
	     (%ui-begin-window "Inspector")
	     (loop for mesh across (scene-meshes scene)
		   doing (when (%ui-treenode (mesh-alias mesh))
			   (loop for instance across (mesh-instances mesh)
				 doing (when (%ui-selectable (instance-alias instance))
					 (setf *selection* instance)))
			   (%ui-treepop)))
	     (%ui-end-window)
	     (instance-properties-panel)
	     (%ui-end)
	     (%renderer-render-end))
    (scene-destroy scene))
  (%renderer-quit))

;; (demo)
;; (setf (instance-transform (aref (mesh-instances (aref (scene-meshes *scene*) 0)) 0)) (transform! 0.0 0.0 0.0 1.0 1.0 1.0 0.0 0.0 0.0))
