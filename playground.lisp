(in-package :playground)

(define-foreign-library libcppengine
    (:unix (:default "./build/libcppengine"))
    (t (:default "./build/libcppengine")))

(unless (foreign-library-loaded-p 'libcppengine)
    (use-foreign-library libcppengine))

(defctype scene-handle :uint64)
(defctype mesh-handle :uint64)
(defctype instance-handle :uint64)

(defcfun ("renderer_init" renderer-init) :bool)

(defcfun ("renderer_scene_create" renderer-scene-create) scene-handle)
(defcfun ("renderer_scene_destroy" renderer-scene-destroy) :bool
  (sh scene-handle))

(defcfun ("renderer_scene_load_obj" renderer-scene-load-obj) :bool
  (sh scene-handle)
  (path :string))

(defcfun ("renderer_scene_remove_mesh" renderer-scene-remove-mesh) :bool
  (sh scene-handle)
  (mh mesh-handle))

(defcfun ("renderer_scene_add_instance" renderer-scene-add-instance) instance-handle
  (sh scene-handle)
  (m mesh-handle)
  (px :float) (py :float) (pz :float)
  (sx :float) (sy :float) (sz :float)
  (rx :float) (ry :float) (rz :float))

(defcfun ("renderer_scene_remove_instance" renderer-scene-remove-instance) :bool
  (sh scene-handle)
  (i instance-handle))

(defcfun ("renderer_sync_scene" renderer-sync-scene) :bool
  (sh scene-handle))
(defcfun ("renderer_render_begin" renderer-render-begin) :void)
(defcfun ("renderer_render" renderer-render) :void)
(defcfun ("renderer_render_end" renderer-render-end) :void)

(defcfun ("renderer_ui_begin" renderer-ui-begin) :void)
(defcfun ("renderer_ui_end" renderer-ui-end) :void)

(defcfun ("ui_begin_window" ui-begin-window)
  :void
  (name :string))
(defcfun ("ui_end_window" ui-end-window)
  :void)
(defcfun ("ui_button" ui-button)
  :bool
  (label :string))

(defcfun ("renderer_clear" renderer-clear) :bool)

(defcfun ("renderer_quit" renderer-quit) :void)

(defcfun ("renderer_poll_events" renderer-poll-events) :void)

(defcfun ("renderer_should_close" renderer-should-close-p) :bool)

(defcfun ("renderer_scene_count" renderer-scene-count) :uint64)

(defcfun ("renderer_scene_at" renderer-scene-at) scene-handle
  (index :uint64))

(defcfun ("renderer_scene_mesh_count" renderer-scene-mesh-count) :uint64
  (sh scene-handle))

(defcfun ("renderer_scene_mesh_at" renderer-scene-mesh-at) mesh-handle
  (sh scene-handle)
  (index :uint64))

(defcfun ("renderer_scene_mesh_instance_count" renderer-scene-mesh-instance-count) :uint64
  (sh scene-handle)
  (mesh mesh-handle))

(defcfun ("renderer_scene_mesh_instance_at" renderer-scene-mesh-instance-at) instance-handle
  (sh scene-handle)
  (mesh mesh-handle)
  (index :uint64))

(defcfun ("renderer_camera_set" renderer-camera-set) :void
  (px :float) (py :float) (pz :float)
  (yaw :float) (pitch :float))

(defcenum (mouse-button :int)
  (:left 1)
  (:middle 2)
  (:right 3)
  (:x1 4)
  (:x2 5))

(defcfun ("renderer_mouse_x" renderer-mouse-x) :float)
(defcfun ("renderer_mouse_y" renderer-mouse-y) :float)
(defcfun ("renderer_mouse_dx" renderer-mouse-dx) :float)
(defcfun ("renderer_mouse_dy" renderer-mouse-dy) :float)
(defcfun ("renderer_mouse_wheel_x" renderer-mouse-wheel-x) :float)
(defcfun ("renderer_mouse_wheel_y" renderer-mouse-wheel-y) :float)
(defcfun ("renderer_mouse_down" renderer-mouse-down-p) :bool
  (button mouse-button))
(defcfun ("renderer_mouse_pressed" renderer-mouse-pressed-p) :bool
  (button mouse-button))
(defcfun ("renderer_mouse_released" renderer-mouse-released-p) :bool
  (button mouse-button))

(cffi:defcfun ("renderer_camera_move" renderer-camera-move) :void
  (forward :float)
  (right :float)
  (up :float))

(cffi:defcfun ("renderer_camera_rotate" renderer-camera-rotate) :void
  (yaw :float)
  (pitch :float))

(defcfun ("renderer_camera_update" renderer-camera-update) :void)

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

(defclass instance ()
  ((handle :reader instance-handle :type u64)
   (transform :accessor instance-transform :initarg :transform
	      :type transform)))

(defclass mesh ()
  ((handle :reader mesh-handle :type u64 :initarg :handle)
   (scene :reader mesh-scene :initarg :scene)
   (instances :reader mesh-instances
              :initform (make-array 0 :adjustable t :fill-pointer 0))))

(defmethod mesh-add-instance ((mesh mesh) (transform transform))
  (let* ((scene (mesh-scene mesh))
         (instance
           (make-instance 'instance
                          :transform transform)))
    (setf (slot-value instance 'handle)
          (renderer-scene-add-instance
           (scene-handle scene)
           (mesh-handle mesh)
           (transform-px transform)
           (transform-py transform)
           (transform-pz transform)
           (transform-sx transform)
           (transform-sy transform)
           (transform-sz transform)
           (transform-rx transform)
           (transform-ry transform)
           (transform-rz transform)))
    (vector-push-extend instance
                        (slot-value mesh 'instances))
    instance))

(defclass scene ()
  ((handle :reader scene-handle :type u64)
   (meshes :reader scene-meshes
           :initform (make-array 0 :adjustable t :fill-pointer 0))))

(defmethod initialize-instance :after ((scene scene) &rest initargs)
  (declare (ignore initargs))
  (setf (slot-value scene 'handle) (renderer-scene-create)))

(defmethod scene-load-obj ((scene scene) path)
  (renderer-scene-load-obj (scene-handle scene) path))

(defmethod scene-meshes ((scene scene))
  (let ((v (slot-value scene 'meshes)))
    (setf (fill-pointer v) 0)
    (dotimes (i (renderer-scene-mesh-count (scene-handle scene)))
      (vector-push-extend
       (make-instance 'mesh
                      :handle (renderer-scene-mesh-at (scene-handle scene) i)
                      :scene scene)
       v))
    v))

(defmethod scene-sync ((scene scene))
  (renderer-sync-scene (scene-handle scene)))

(defmethod scene-destroy ((scene scene))
  (renderer-scene-destroy (scene-handle scene)))

(defmethod mesh-instances ((mesh mesh))
  (let ((scene (mesh-scene mesh))
        (v (slot-value mesh 'instances)))
    (setf (fill-pointer v) 0)
    (dotimes (i (renderer-scene-mesh-instance-count (scene-handle scene) (mesh-handle mesh)))
      (vector-push-extend
       (make-instance 'instance
                      :handle (renderer-scene-mesh-instance-at
                               (scene-handle scene) (mesh-handle mesh) i))
       v))
    v))

(defun demo ()
  (renderer-init)
  (let ((scene (renderer-scene-create)))
    (renderer-scene-load-obj scene "models/cube.obj")
    (let ((count (renderer-scene-mesh-count scene)))
      (dotimes (i count)
	(let ((mesh (renderer-scene-mesh-at scene i)))
          (renderer-scene-add-instance
           scene mesh
           0.0 0.0 0.0
           1.0 1.0 1.0
           0.0 0.0 0.0))))
    (renderer-sync-scene scene)
    (loop until (renderer-should-close-p)
	  doing
	     (renderer-poll-events)
	     (renderer-render))
    (renderer-scene-destroy scene))
  (renderer-quit))

;; (demo)

;; Letters
(defconstant +key-a+ 4)
(defconstant +key-b+ 5)
(defconstant +key-c+ 6)
(defconstant +key-d+ 7)
(defconstant +key-e+ 8)
(defconstant +key-f+ 9)
(defconstant +key-g+ 10)
(defconstant +key-h+ 11)
(defconstant +key-i+ 12)
(defconstant +key-j+ 13)
(defconstant +key-k+ 14)
(defconstant +key-l+ 15)
(defconstant +key-m+ 16)
(defconstant +key-n+ 17)
(defconstant +key-o+ 18)
(defconstant +key-p+ 19)
(defconstant +key-q+ 20)
(defconstant +key-r+ 21)
(defconstant +key-s+ 22)
(defconstant +key-t+ 23)
(defconstant +key-u+ 24)
(defconstant +key-v+ 25)
(defconstant +key-w+ 26)
(defconstant +key-x+ 27)
(defconstant +key-y+ 28)
(defconstant +key-z+ 29)

;; Numbers
(defconstant +key-1+ 30)
(defconstant +key-2+ 31)
(defconstant +key-3+ 32)
(defconstant +key-4+ 33)
(defconstant +key-5+ 34)
(defconstant +key-6+ 35)
(defconstant +key-7+ 36)
(defconstant +key-8+ 37)
(defconstant +key-9+ 38)
(defconstant +key-0+ 39)

;; Common
(defconstant +key-enter+ 40)
(defconstant +key-escape+ 41)
(defconstant +key-backspace+ 42)
(defconstant +key-tab+ 43)
(defconstant +key-space+ 44)

;; Punctuation
(defconstant +key-minus+ 45)
(defconstant +key-equals+ 46)
(defconstant +key-left-bracket+ 47)
(defconstant +key-right-bracket+ 48)
(defconstant +key-backslash+ 49)
(defconstant +key-semicolon+ 51)
(defconstant +key-apostrophe+ 52)
(defconstant +key-grave+ 53)
(defconstant +key-comma+ 54)
(defconstant +key-period+ 55)
(defconstant +key-slash+ 56)

;; Function keys
(defconstant +key-f1+ 58)
(defconstant +key-f2+ 59)
(defconstant +key-f3+ 60)
(defconstant +key-f4+ 61)
(defconstant +key-f5+ 62)
(defconstant +key-f6+ 63)
(defconstant +key-f7+ 64)
(defconstant +key-f8+ 65)
(defconstant +key-f9+ 66)
(defconstant +key-f10+ 67)
(defconstant +key-f11+ 68)
(defconstant +key-f12+ 69)

;; Navigation
(defconstant +key-insert+ 73)
(defconstant +key-home+ 74)
(defconstant +key-page-up+ 75)
(defconstant +key-delete+ 76)
(defconstant +key-end+ 77)
(defconstant +key-page-down+ 78)
(defconstant +key-right+ 79)
(defconstant +key-left+ 80)
(defconstant +key-down+ 81)
(defconstant +key-up+ 82)

;; Modifiers
(defconstant +key-left-ctrl+ 224)
(defconstant +key-left-shift+ 225)
(defconstant +key-left-alt+ 226)
(defconstant +key-left-gui+ 227)
(defconstant +key-right-ctrl+ 228)
(defconstant +key-right-shift+ 229)
(defconstant +key-right-alt+ 230)
(defconstant +key-right-gui+ 231)

(cffi:defcfun ("renderer_key_down" renderer-key-down-p) :bool
  (key :int))

(cffi:defcfun ("renderer_key_pressed" renderer-key-pressed-p) :bool
  (key :int))

(cffi:defcfun ("renderer_key_released" renderer-key-released-p) :bool
  (key :int))

(cffi:defcfun ("renderer_dt" renderer-dt) :float)

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

(defun demo2 ()
  (renderer-init)
  (let ((scene (make-instance 'scene)))
    (scene-load-obj scene "models/bmw.obj")
    (loop for mesh across (scene-meshes scene)
	  doing
	     (mesh-add-instance mesh (transform! 500.0 -10.0 0.0 1.0 1.0 1.0 0.0 0.0 0.0)))
    (scene-sync scene)
    (loop until (renderer-should-close-p)
	  for dt = (renderer-dt)
	  doing
	     (renderer-poll-events)
	     (let* ((wheel-y (renderer-mouse-wheel-y))
		    (wheel-x (renderer-mouse-wheel-x))
		    (dx (renderer-mouse-dx))
		    (dy (renderer-mouse-dy))
		    (w-down (renderer-key-down-p +key-w+))
		    (a-down (renderer-key-down-p +key-a+))
		    (s-down (renderer-key-down-p +key-s+))
		    (d-down (renderer-key-down-p +key-d+))
		    (space-down (renderer-key-down-p +key-space+))
		    (ctrl-down (renderer-key-down-p +key-left-ctrl+))
		    (move-speed (* *keyboard-sensitivity* dt))
		    (look-speed (* *mouse-sensitivity* dt))
		    (middle-click (renderer-mouse-down-p :middle)))
	       (renderer-camera-rotate (* (- wheel-x) look-speed) (* wheel-y look-speed))
	       (renderer-camera-move
		(+ (if w-down move-speed 0.0)
		   (if s-down (- move-speed) 0.0))
		(if middle-click
		    (* (- dx) look-speed)
		    0.0)
		(if middle-click
		    (* dy look-speed)
		    0.0)))
	     (renderer-camera-update)
	     (renderer-render-begin)
	     (renderer-render)
	     (renderer-ui-begin)
	     (ui-begin-window "Window")
	     (loop for mesh across (scene-meshes scene)
		   doing (when (ui-button (format nil "Mesh #~a~%" (mesh-handle mesh)))
			   (format t "Mesh #~a clicked~%" (mesh-handle mesh))))
	     (ui-end-window)
	     (renderer-ui-end)
	     (renderer-render-end))
    (scene-destroy scene))
  (renderer-quit))

(demo2)
