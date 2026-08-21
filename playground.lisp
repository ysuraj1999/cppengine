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

(defcfun ("renderer_render" renderer-render) :void)

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

(defclass instance ()
  ((handle :reader instance-handle :type u64)
   (transform :accessor instance-transform :initarg :transform)))

(defclass mesh ()
  ((handle :reader mesh-handle :type u64 :initarg :handle)
   (scene :reader mesh-scene :initarg :scene)
   (instances :reader mesh-instances
              :initform (make-array 0 :adjustable t :fill-pointer 0))))

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

(defmacro with-transform ((px py pz sx sy sz rx ry rz) transform &body body)
  `(let ((,px (elt ,transform 0))
         (,py (elt ,transform 1))
         (,pz (elt ,transform 2))
         (,sx (elt ,transform 3))
         (,sy (elt ,transform 4))
         (,sz (elt ,transform 5))
         (,rx (elt ,transform 6))
         (,ry (elt ,transform 7))
         (,rz (elt ,transform 8)))
     ,@body))

(defmethod scene-add-instance ((scene scene) (mesh mesh) (inst instance))
  (with-transform (px py pz sx sy sz rx ry rz) (instance-transform inst)
    (setf (slot-value inst 'handle)
          (renderer-scene-add-instance
           (scene-handle scene) (mesh-handle mesh)
           px py pz sx sy sz rx ry rz)))
  (vector-push-extend inst (slot-value mesh 'instances))
  inst)

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

(defun clamp(val minimum maximum)
  (min (max val minimum) maximum))

(defun wrap(value maximum)
  (if (>= value maximum)
      0.0
      value))

(defparameter *mouse-sensitivity* 5.0)
(defparameter *keyboard-sensitivity* 1.0)

(defun demo2 ()
  (renderer-init)
  (let ((scene (make-instance 'scene)))
    (scene-load-obj scene "models/bmw.obj")
    (loop for mesh across (scene-meshes scene)
	  doing
	     (scene-add-instance
	      scene mesh
	      (make-instance 'instance
			     :transform '(500.0 -10.0 0.0 1.0 1.0 1.0 0.0 0.0 0.0))))
    (scene-sync scene)
    (loop until (renderer-should-close-p)
	  doing
	     (renderer-poll-events)
	     (let ((dy (renderer-mouse-wheel-y))
		   (dx (renderer-mouse-wheel-x)))
	       (renderer-camera-rotate (- dx) dy)
	       (renderer-camera-update))
	     (renderer-render))
    (scene-destroy scene))
  (renderer-quit))

(demo2)
