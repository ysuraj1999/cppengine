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
    (:right-ctrl 228) (:right-shift 229) (:right-alt 230) (:right-gui 231)
    ;; Convenience to combine righ and left modifiers, use left
    (:ctrl 224) (:shift 225) (:alt 226) (:gui 227)))

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

(defcfun ("renderer_rebuild_scene" %renderer-rebuild-scene) :bool
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
  (label :string)
  (selected :bool))

(defcfun ("ui_sameline" %ui-sameline)
    :void)

(defcfun ("ui_text" %ui-text)
    :void
  (text :string))

(defcfun ("ui_push_id" %ui-push-id)
    :void
  (id :int))

(defcfun ("ui_pop_id" %ui-pop-id)
    :void)

(defcfun ("ui_begin_context_menu_item" %ui-begin-context-menu-item)
    :bool)

(defcfun ("ui_begin_context_menu_window" %ui-begin-context-menu-window)
    :bool)

(defcfun ("ui_end_context_menu" %ui-end-context-menu)
    :void)

(defcfun ("ui_menu_item" %ui-menu-item)
    :bool
  (label :string))

(defcfun ("renderer_clear" %renderer-clear) :bool)

(defcfun ("renderer_quit" %renderer-quit) :void)

(defcfun ("input_poll" %input-poll) :void)

(defcfun ("input_should_close" %input-should-close-p) :bool)

(defcfun ("input_set_cursor_captured" %input-set-cursor-captured) :void
  (captured :bool))

(defcfun ("input_cursor_captured" %input-cursor-captured-p) :bool)

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
(defun %input-mouse-up-p(button)
  (not (%input-mouse-down-p button)))

(defcfun ("input_mouse_pressed" %input-mouse-pressed-p) :bool
  (button %mouse-button))
(defcfun ("input_mouse_released" %input-mouse-released-p) :bool
  (button %mouse-button))

(defcenum (%mouse-event-type :int)
  (:motion 1)
  (:button 2)
  (:wheel 3))

(defcstruct (%mouse-event :class cffi-mouse-event)
    (type %mouse-event-type)
  (button :int)
  (down :bool)
  (x :float)
  (y :float)
  (dx :float)
  (dy :float)
  (wheel-x :float)
  (wheel-y :float))

(defcfun ("input_mouse_event_count" %input-mouse-event-count) :uint64)

(defcfun ("input_mouse_event_at" %input-mouse-event-at)
    (:struct %mouse-event)
  (index :uint64))

(defstruct mouse-event
  (type :motion :type symbol)
  (button 0 :type (signed-byte 32))
  (down nil :type boolean)
  (x 0.0 :type single-float)
  (y 0.0 :type single-float)
  (dx 0.0 :type single-float)
  (dy 0.0 :type single-float)
  (wheel-x 0.0 :type single-float)
  (wheel-y 0.0 :type single-float))

(defmethod translate-from-foreign (ptr (type cffi-mouse-event))
  (make-mouse-event
   :type (foreign-slot-value ptr '(:struct %mouse-event) 'type)
   :button (foreign-slot-value ptr '(:struct %mouse-event) 'button)
   :down (foreign-slot-value ptr '(:struct %mouse-event) 'down)
   :x (foreign-slot-value ptr '(:struct %mouse-event) 'x)
   :y (foreign-slot-value ptr '(:struct %mouse-event) 'y)
   :dx (foreign-slot-value ptr '(:struct %mouse-event) 'dx)
   :dy (foreign-slot-value ptr '(:struct %mouse-event) 'dy)
   :wheel-x (foreign-slot-value ptr '(:struct %mouse-event) 'wheel-x)
   :wheel-y (foreign-slot-value ptr '(:struct %mouse-event) 'wheel-y)))

(defcfun ("renderer_camera_move" %renderer-camera-move) :void
  (forward :float)
  (right :float)
  (up :float))

(defcfun ("renderer_camera_rotate" %renderer-camera-rotate) :void
  (yaw :float)
  (pitch :float))

(defcfun ("input_key_down" %input-key-down-p) :bool
  (key :int))
(defun %input-key-up-p(key)
  (not (%input-key-down-p key)))

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

(defstruct keyboard-event
  (scancode 0 :type (signed-byte 32))
  (down nil :type boolean)
  (repeat nil :type boolean))

(defmethod translate-from-foreign (ptr (type cffi-keyboard-event))
  (make-keyboard-event
   :scancode (foreign-slot-value ptr '(:struct %keyboard-event) 'scancode)
   :down     (foreign-slot-value ptr '(:struct %keyboard-event) 'down)
   :repeat   (foreign-slot-value ptr '(:struct %keyboard-event) 'repeat)))

(defun keyboard-events ()
  "Return a typed vector of this frame's key events in the order SDL
delivered them; each element is a KEYBOARD-EVENT with SCANCODE, DOWN and
REPEAT."
  (let ((count (%input-keyboard-event-count)))
    (loop with events = (make-array count
                                    :element-type 'keyboard-event
                                    :fill-pointer 0)
          for i below count
          do (vector-push-extend (%input-keyboard-event-at i) events)
          finally (return events))))

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
