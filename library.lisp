(in-package :playground)

(define-foreign-library libcppengine
    (:unix (:default "./build/libcppengine"))
  (t (:default "./build/libcppengine")))

(unless (foreign-library-loaded-p 'libcppengine)
  (use-foreign-library libcppengine))

(defctype %mesh-handle :uint64)
(defctype %instance-handle :uint64)

(defcstruct (%transform :class cffi-transform)
    (px :float) (py :float) (pz :float)
  (sx :float) (sy :float) (sz :float)
  (rx :float) (ry :float) (rz :float))

;; Mirrors the canonical C Key enum in cppengine.h; numeric values are
;; carried through unchanged.
(defcenum (%key :int)
    (:unknown 0)
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
  (:right-ctrl 228) (:right-shift 229) (:right-alt 230) (:right-gui 231)
  ;; Convenience aliases that resolve to the left modifier
  (:ctrl 224) (:shift 225) (:alt 226) (:gui 227))

(defun key-code (key)
  "Numeric Key code for KEY keyword; mirrors the canonical C Key enum."
  (foreign-enum-value '%key key))

(defcfun ("renderer_init" c-renderer-init) :bool)

(defcfun ("renderer_load_obj" c-renderer-load-obj) :uint64
  (path :string))

(defcfun ("remove_mesh" c-remove-mesh) :bool
  (mh %mesh-handle))

(defcfun ("remove_instance" c-remove-instance) :bool
  (i %instance-handle))

(defcfun ("add_instance" c-add-instance) %instance-handle
  (m %mesh-handle)
  (xform (:struct %transform)))

;; Returns a pointer into engine-owned memory; do not free.
(defcfun ("mesh_alias" c-mesh-alias) :pointer
  (mh %mesh-handle))

(defun mesh-alias (mh)
  (foreign-string-to-lisp (c-mesh-alias mh)))

;; Returns a pointer into engine-owned memory; do not free.
(defcfun ("instance_alias" c-instance-alias) :pointer
  (ih %instance-handle))

(defun instance-alias (ih)
  (foreign-string-to-lisp (c-instance-alias ih)))

(defcfun ("instance_get_transform" c-instance-get-transform)
    (:struct %transform)
  (i %instance-handle))

(defcfun ("instance_set_transform"
          c-instance-set-transform) :void
  (i :uint64)
  (xform (:struct %transform)))

(defcfun ("mesh_count" c-mesh-count) :uint64)
(defcfun ("mesh_at" c-mesh-at) %mesh-handle
  (index :uint64))
(defcfun ("mesh_instance_count" c-mesh-instance-count) :uint64
  (mesh %mesh-handle))
(defcfun ("mesh_instance_at" c-mesh-instance-at) %instance-handle
  (mesh %mesh-handle)
  (index :uint64))

(defcfun ("ensure_instance_count" c-ensure-instance-count) :void
  (mesh %mesh-handle)
  (n :uint64))

(defun ensure-instance-count (mesh n)
  "Ensure MESH has exactly N instances. Creates missing, prunes excess, no-op if equal."
  (c-ensure-instance-count mesh n))

(defcfun ("renderer_rebuild" c-renderer-rebuild) :void)
(defcfun ("renderer_render_begin" c-renderer-render-begin) :void)
(defcfun ("renderer_render_scene" c-renderer-render-scene) :void)
(defcfun ("renderer_render_end" c-renderer-render-end) :void)
(defcfun ("renderer_render" c-renderer-render) :void)

(defcfun ("ui_begin" c-ui-begin) :void)
(defcfun ("ui_end" c-ui-end) :void)

(defcfun ("ui_begin_window" c-ui-begin-window)
  :void
  (name :string))
(defcfun ("ui_end_window" c-ui-end-window)
  :void)
(defcfun ("ui_button" c-ui-button)
  :bool
  (label :string))

(defcfun ("ui_begin_tree" c-ui-begin-tree)
  :bool
  (label :string))

(defcfun ("ui_end_tree" c-ui-end-tree)
  :void)

(defcfun ("ui_selectable" c-ui-selectable)
  :bool
  (label :string)
  (selected :bool))

(defcfun ("ui_sameline" c-ui-sameline)
  :void)

(defcfun ("ui_text" c-ui-text)
  :void
  (text :string))

(defcfun ("ui_push_id" c-ui-push-id)
  :void
  (id :int))

(defcfun ("ui_pop_id" c-ui-pop-id)
  :void)

(defcfun ("ui_begin_context_menu_item" c-ui-begin-context-menu-item)
  :bool)

(defcfun ("ui_begin_context_menu_window" c-ui-begin-context-menu-window)
  :bool)

(defcfun ("ui_end_context_menu" c-ui-end-context-menu)
  :void)

(defcfun ("ui_menu_item" c-ui-menu-item)
  :bool
  (label :string))

(defcfun ("renderer_clear" c-renderer-clear) :bool)

(defcfun ("renderer_quit" c-renderer-quit) :void)

(defcfun ("input_poll" c-input-poll) :void)

(defcfun ("input_should_close" c-input-should-close-p) :bool)

(defcfun ("input_set_cursor_captured" c-input-set-cursor-captured) :void
  (captured :bool))

(defcfun ("input_cursor_captured" c-input-cursor-captured-p) :bool)

(defcfun ("renderer_camera_set" c-renderer-camera-set) :void
  (px :float) (py :float) (pz :float)
  (yaw :float) (pitch :float))

(defcenum (%mouse-button :int)
    (:left 1)
  (:none 0)
  (:middle 2)
  (:right 3)
  (:x1 4)
  (:x2 5))

(defcfun ("ui_item_double_clicked" c-ui-item-double-clicked)
  :bool
  (button %mouse-button))

(defcfun ("ui_item_clicked" c-ui-item-clicked)
  :bool
  (button %mouse-button))


(defun ui-button (label)
  "Submit a button as the current item.  Returns the underlying click
result, which callers normally ignore; attach handlers with UI-ON-CLICK."
  (c-ui-button label))

(defun ui-selectable (label selected-p)
  "Submit a selectable as the current item.  SELECTED-P is the displayed
selection state.  Returns the underlying click result, which callers
normally ignore; attach handlers with UI-ON-CLICK."
  (c-ui-selectable label selected-p))

(defmacro ui-on-click (button &body body)
  "Run BODY when the current item is clicked with mouse BUTTON
(:left, :middle, :right, :x1 or :x2)."
  `(when (c-ui-item-clicked ,button)
     ,@body))

(defmacro ui-on-double-click (button &body body)
  "Run BODY when the current item is double-clicked with mouse BUTTON."
  `(when (c-ui-item-double-clicked ,button)
     ,@body))

(defmacro ui-on-context (&body body)
  "Open the context popup for the current item, run BODY (normally
UI-MENU-ITEM forms), and close the popup."
  `(when (c-ui-begin-context-menu-item)
     ,@body
     (c-ui-end-context-menu)))

(defmacro ui-on-context-window (&body body)
  "Open the context popup of the current window (not bound to a specific
item), run BODY, and close the popup."
  `(when (c-ui-begin-context-menu-window)
     ,@body
     (c-ui-end-context-menu)))

(defmacro ui-menu-item (label &body body)
  "Submit a menu item inside a context popup; run BODY when it is clicked."
  `(when (c-ui-menu-item ,label)
     ,@body))

(defcfun ("input_mouse_x" c-input-mouse-x) :float)
(defcfun ("input_mouse_y" c-input-mouse-y) :float)
(defcfun ("input_mouse_dx" c-input-mouse-dx) :float)
(defcfun ("input_mouse_dy" c-input-mouse-dy) :float)
(defcfun ("input_mouse_wheel_x" c-input-mouse-wheel-x) :float)
(defcfun ("input_mouse_wheel_y" c-input-mouse-wheel-y) :float)
(defcfun ("input_mouse_down" c-input-mouse-down-p) :bool
  (button %mouse-button))
(defcfun ("input_mouse_up" c-input-mouse-up-p) :bool
  (button %mouse-button))

(defcfun ("input_mouse_pressed" c-input-mouse-pressed-p) :bool
  (button %mouse-button))
(defcfun ("input_mouse_released" c-input-mouse-released-p) :bool
  (button %mouse-button))

(defcenum (%input-event-type :int)
    (:key-down 1)
  (:key-up 2)
  (:mouse-motion 3)
  (:mouse-button-down 4)
  (:mouse-button-up 5)
  (:mouse-wheel 6))

(defcstruct (%input-event :class cffi-input-event)
    (type %input-event-type)
  (key %key)
  (button %mouse-button)
  (down :bool)
  (repeat :bool)
  (x :float)
  (y :float)
  (dx :float)
  (dy :float)
  (wheel-x :float)
  (wheel-y :float))

(defcfun ("input_event_count" c-input-event-count) :uint64)

(defcfun ("input_event_at" c-input-event-at)
    (:struct %input-event)
  (index :uint64))

(defstruct input-event
  (type :key-down :type symbol)
  (key :unknown :type symbol)
  (button :none :type symbol)
  (down nil :type boolean)
  (repeat nil :type boolean)
  (x 0.0 :type single-float)
  (y 0.0 :type single-float)
  (dx 0.0 :type single-float)
  (dy 0.0 :type single-float)
  (wheel-x 0.0 :type single-float)
  (wheel-y 0.0 :type single-float))

(defmethod translate-from-foreign (ptr (type cffi-input-event))
  (make-input-event
   :type    (foreign-slot-value ptr '(:struct %input-event) 'type)
   :key     (foreign-slot-value ptr '(:struct %input-event) 'key)
   :button  (foreign-slot-value ptr '(:struct %input-event) 'button)
   :down    (foreign-slot-value ptr '(:struct %input-event) 'down)
   :repeat  (foreign-slot-value ptr '(:struct %input-event) 'repeat)
   :x       (foreign-slot-value ptr '(:struct %input-event) 'x)
   :y       (foreign-slot-value ptr '(:struct %input-event) 'y)
   :dx      (foreign-slot-value ptr '(:struct %input-event) 'dx)
   :dy      (foreign-slot-value ptr '(:struct %input-event) 'dy)
   :wheel-x (foreign-slot-value ptr '(:struct %input-event) 'wheel-x)
   :wheel-y (foreign-slot-value ptr '(:struct %input-event) 'wheel-y)))

(defun input-events ()
  "Return a typed vector of this frame's input events in the order SDL
    delivered them; each element is an INPUT-EVENT with TYPE, KEY, BUTTON
    (keywords), DOWN, REPEAT and the mouse position/delta/wheel fields."
  (let ((count (c-input-event-count)))
    (loop with events = (make-array count
                                    :element-type 'input-event
                                    :fill-pointer 0)
          for i below count
          do (vector-push-extend (c-input-event-at i) events)
          finally (return events))))

(defcfun ("renderer_camera_move" c-renderer-camera-move) :void
  (forward :float)
  (right :float)
  (up :float))

(defcfun ("renderer_camera_rotate" c-renderer-camera-rotate) :void
  (yaw :float)
  (pitch :float))

(defcfun ("renderer_camera_look_at" c-renderer-camera-look-at) :void
  (x :float)
  (y :float)
  (z :float))

(defcfun ("input_key_down" c-input-key-down-p) :bool
  (key %key))
(defcfun ("input_key_up" c-input-key-up-p) :bool
  (key %key))

(defcfun ("input_key_pressed" c-input-key-pressed-p) :bool
  (key %key))

(defcfun ("input_key_released" c-input-key-released-p) :bool
  (key %key))


(defcfun ("time_dt" c-time-dt) :float)


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

;; The renderer owns a single implicit scene; the mesh/instance handles are
;; the public references to objects inside it.

(defun load-obj (path)
  "Load an OBJ into the renderer's single scene.  Returns the number of
meshes added; traverse them with DO-MESHES."
  (c-renderer-load-obj path))

(defun instance (mesh)
  "Create a new instance of MESH in the single scene at the origin with
unit scale.  Returns an InstanceHandle."
  (c-add-instance mesh (transform! 0.0 0.0 0.0 1.0 1.0 1.0 0.0 0.0 0.0)))

(defun instance-transform (ih)
  (c-instance-get-transform ih))

(defun (setf instance-transform) (value ih)
  (c-instance-set-transform ih value)
  value)

(defun remove-mesh (mh)
  (c-remove-mesh mh))

(defun remove-instance (ih)
  (c-remove-instance ih))

(defun render ()
  "Render the renderer's single implicit scene."
  (c-renderer-render))
