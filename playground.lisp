(in-package :playground)

(deftype u64 () '(unsigned-byte 64))

(defvar *mesh-objects* (make-hash-table :test #'eql))
(defvar *instance-objects* (make-hash-table :test #'eql))
(defvar *scene-objects* (make-hash-table :test #'eql))

(defvar *selection* nil)
(defvar *current-scene* nil)

(defun selection-get (object)
  (eq *selection* object))

(defun selection-set (object)
  (setf *selection* object)
  object)

(defun ensure-mesh (scene handle)
  (or (gethash handle *mesh-objects*)
      (setf (gethash handle *mesh-objects*)
            (make-instance 'mesh :handle handle :scene scene))))

(defun ensure-instance (mesh handle)
  (or (gethash handle *instance-objects*)
      (setf (gethash handle *instance-objects*)
            (make-instance 'instance :handle handle :mesh mesh))))

(defun ensure-scene (handle)
  (or (gethash handle *scene-objects*)
      (let ((scene (allocate-instance (find-class 'scene))))
        (setf (slot-value scene 'handle) handle)
        (setf (gethash handle *scene-objects*) scene))))

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

(defun scene-handles ()
  "Snapshot of every live scene handle in creation order; safe to
iterate while scenes are created or destroyed."
  (loop with count = (%scene-count)
        with handles = (make-array count :fill-pointer 0)
        for i below count
        do (vector-push-extend (%scene-at i) handles)
        finally (return handles)))

(defmethod scene-sync ((scene scene))
  (%renderer-rebuild-scene (scene-handle scene)))

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
      (remhash (instance-handle instance) *instance-objects*))
    (when (member *selection* dead-instances)
      (setf *selection* nil))
    (when (eq *current-scene* scene)
      (setf *current-scene* nil))
    (remhash (scene-handle scene) *scene-objects*)))

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
      (remhash (instance-handle instance) *instance-objects*)
      (when (eq *selection* instance)
	(setf *selection* nil)))
    ok))

(defstruct prefix-node
  (table (make-hash-table :test #'equal))
  function)

(defun make-prefix-machine()
  (let* ((prefix-root (make-prefix-node))
	 (prefix-cursor prefix-root))
    (labels ((shortcut(keychords)
	       "Convert ((:ctrl x) (:ctrl :f)) => ((9 224) (27 224))"

	       (mapcar #'(lambda(chord)
			   (sort (mapcar #'key chord) #'<))
		       keychords))
	     (add-shortcut(shortcut function)
	       "Attach ((0 1) (2 3)) with function"
	       
	       (let ((cursor prefix-root))
		 (dolist (chord shortcut)
		   (let ((next-node))
		     (setf next-node (or ; existing node or create one
				      (gethash chord (prefix-node-table cursor))
				      (setf (gethash chord (prefix-node-table cursor)) (make-prefix-node))))
		     (setf cursor next-node)))
		 (setf (prefix-node-function cursor) function)))
	     (reset-cursor()
	       (setf prefix-cursor prefix-root))
	     (cancel-action()
	       (reset-cursor)
	       (print "Cancelled"))
	     (move-cursor(node)
	       (setf prefix-cursor node))
	     (invalid-chord()
	       (cancel-action)))
       (lambda (action &rest args)
	 (ecase action
	   (:add-shortcut (destructuring-bind (shortcut-list-expr function) args
			    (add-shortcut (shortcut shortcut-list-expr) function)))
	   (:feed (destructuring-bind (chord) args
		    (if (equal chord (list (key :escape)))
			(cancel-action)
			(let ((next-node (gethash chord (prefix-node-table prefix-cursor))))
			  (if next-node
			      (progn (move-cursor next-node)
				     (when (prefix-node-function prefix-cursor)
				       (funcall (prefix-node-function prefix-cursor))
				       (reset-cursor)))
			      (invalid-chord))))))
	   (:reset (setf prefix-cursor prefix-root))
	   (:debug-root prefix-root))))))
	 
(defparameter *modifier-keys* (mapcar #'key '(:left-ctrl :left-shift :left-alt :left-gui
					      :right-ctrl :right-shift :right-alt :right-gui
					      :ctrl :shift :alt :gui)))
(defun modifier-key-p(scancode)
  (member scancode *modifier-keys*))

(defun make-chord-machine(on-chord)
  (let ((held-keys)
	(held-modifiers)
	(chord-keys)
	(has-key))
    (flet ((add-key(scancode)
	     (if (modifier-key-p scancode)
		 (pushnew scancode held-modifiers)
		 (progn (pushnew scancode held-keys)
			(setf has-key t)))
	     (pushnew scancode chord-keys))
	   (remove-key(scancode)
	     (if (modifier-key-p scancode)
		 (setf held-modifiers (delete scancode held-modifiers))
		 (setf held-keys (delete scancode held-keys)))
	     (when (and (null held-keys) has-key)
	       (setf chord-keys
		     (union held-modifiers chord-keys))
	       (funcall on-chord (sort chord-keys #'<))
	       (setf chord-keys nil
		     has-key nil))))
      (lambda(action &rest args)
	(ecase action
	  (:feed (destructuring-bind (key-event) args
		   (mu:with-members (scancode down repeat) key-event keyboard-event
		     (if down
			 (add-key scancode)
			 (remove-key scancode)))))
	  (:reset (setf held-keys nil
			held-modifiers nil
			chord-keys nil 
			has-key nil)))))))

(defun make-command-input()
  "Single dispatch lambda — mirrors chord/prefix machines.
   Interfaces: :feed (keyboard-event), :reset, :add-shortcut (expr fn),
               :debug-root, :toggle (&optional state), :enabled"
  (let* ((prefix-machine (make-prefix-machine))
	 (enabled t)
	 (chord-machine (make-chord-machine
			 (lambda (chord)
			   (when enabled
			     (funcall prefix-machine :feed chord))))))
    (lambda (action &rest args)
      (ecase action
	(:feed (destructuring-bind (ev) args
		 (when enabled
		   (funcall chord-machine :feed ev))))
	(:reset (funcall chord-machine :reset)
	 (funcall prefix-machine :reset))
	(:add-shortcut (destructuring-bind (expr fn) args
			 (funcall prefix-machine :add-shortcut expr fn)))
	(:debug-root (funcall prefix-machine :debug-root))
	(:toggle (destructuring-bind (&optional (state nil state-supplied-p)) args
		   (if state-supplied-p
		       (setf enabled (if state t nil))
		       (setf enabled (not enabled)))
		   enabled))
	(:enabled enabled)))))

(defun clamp(val minimum maximum)
  (min (max val minimum) maximum))

(defun wrap(value maximum)
  (if (>= value maximum)
      0.0
      value))

;; (require :sb-sprof)
;; (defmacro sbcl-profile(&body body)
;;   `(sb-sprof:with-profiling (:report :flat)
;;      ,@body))

(defun file-picker ()
  (let* ((raw (uiop:run-program '("zenity" "--file-selection")
                                :output :string :ignore-error-status t))
         (path (and raw (string-trim '(#\Newline #\Return #\Space) raw))))
    (and path (plusp (length path)) path)))

(defun panel-instance-properties ()
  (when *selection*
    (%ui-begin-window "Instance Properties")
    (let ((tr (instance-transform *selection*)))
      (mu:with-members (px py pz rx ry rz sx sy sz) tr transform
	(%ui-text (format nil "Position: ~,2f ~,2f ~,2f" px py pz))
	(%ui-text (format nil "Rotation: ~,2f ~,2f ~,2f" rx ry rz))
	(%ui-text (format nil "Scale: ~,2f ~,2f ~,2f" sx sy sz))))
    (%ui-end-window)))

(defun panel-inspector ()
  (%ui-begin-window "Inspector")
  (let ((delete-scene nil))
    (do-scenes (scene)
      (%ui-push-id (scene-handle scene))
      (let ((scene-open (%ui-treenode (format nil "Scene ~a" (scene-handle scene)))))
        (when (%ui-begin-context-menu-item)
          (when (%ui-menu-item "Push Scene")
            (setf *current-scene* scene)
            (scene-sync scene))
          (when (%ui-menu-item "Instantiate All Meshes")
            (do-scene-meshes (mesh scene)
              (mesh-add-instance mesh (transform! 0.0 0.0 0.0 1.0 1.0 1.0 0.0 0.0 0.0))))
          (when (%ui-menu-item "Load OBJ")
            (let ((path (file-picker)))
              (when path
                (scene-load-obj scene path))))
          (when (%ui-menu-item "Delete Scene")
            (setf delete-scene scene))
          (%ui-end-context-menu))
        (when scene-open
          (do-scene-meshes (mesh scene)
            (%ui-push-id (mesh-handle mesh))
            (let ((mesh-open (%ui-treenode (mesh-alias mesh))))
              (when (%ui-begin-context-menu-item)
                (when (%ui-menu-item "Instantiate")
                  (mesh-add-instance mesh (transform! 0.0 0.0 0.0 1.0 1.0 1.0 0.0 0.0 0.0)))
                (%ui-end-context-menu))
              (when mesh-open
                (do-mesh-instances (instance mesh)
                  (%ui-push-id (instance-handle instance))
                  (when (%ui-selectable (instance-alias instance)
                                        (selection-get instance))
                    (selection-set instance))
                  (when (%ui-begin-context-menu-item)
                    (when (%ui-menu-item "Delete")
                      (scene-remove-instance scene instance))
                    (%ui-end-context-menu))
                  (%ui-pop-id))
                (%ui-treepop)))
            (%ui-pop-id))
          (%ui-treepop)))
      (%ui-pop-id))
    (when delete-scene
      (scene-destroy delete-scene))
    (when (%ui-begin-context-menu-window)
      (when (%ui-menu-item "New Scene")
        (make-instance 'scene))
      (%ui-end-context-menu))
    (%ui-end-window)))


(defparameter *keyboard-sensitivity* 10.0)
(defparameter *mouse-sensitivity* 10.0)

(defvar *interaction-stack* '()
  "Stack of active interactions; the head (car) is the current one.
   An interaction is the interface `editor` talks to — fed tick and
   keyboard/mouse events directly. The current interaction receives all
   input; other interactions on the stack are quiescent until targeted.")

(defun push-interaction (it)
  (push it *interaction-stack*)
  (funcall it :enter))

(defun exit-interaction ()
  "Pop the top (current) interaction off the stack.
   Assumes the exiting interaction is the current one — it pops itself.
   Guarded so the bottom (fundamental) interaction is never popped."
  (when (> (length *interaction-stack*) 1)
    (let ((it (first *interaction-stack*)))
      (funcall it :exit)
      (pop *interaction-stack*))))

(defun current-interaction ()
  (first *interaction-stack*))

(defun make-interaction (&key tick keyboard-event mouse-event on-enter on-exit)
  "Build an interaction dispatch lambda (action &rest args) handling
   :tick (dt), :keyboard-event (event), :mouse-event (event),
   :enter, :exit. Unprovided callbacks default to no-ops."
  (let ((tick-fn   (or tick (lambda (dt) (declare (ignore dt)))))
        (key-fn    (or keyboard-event (lambda (event) (declare (ignore event)))))
        (mouse-fn  (or mouse-event (lambda (event) (declare (ignore event)))))
        (enter-fn  (or on-enter (lambda ())))
        (exit-fn   (or on-exit (lambda ()))))
    (lambda (action &rest args)
      (ecase action
        (:tick           (apply tick-fn args))
        (:keyboard-event (apply key-fn args))
        (:mouse-event    (apply mouse-fn args))
        (:enter          (funcall enter-fn))
        (:exit           (funcall exit-fn))))))

(defun make-fundamental-2d-interaction ()
  (let* ((command-input (make-command-input)))
    (funcall command-input :add-shortcut '((:ctrl :s)) (lambda () (print "Save")))
    (funcall command-input :add-shortcut '((:ctrl :k)) (lambda () (print "Kill")))
    (funcall command-input :add-shortcut '((:ctrl :p)) (lambda () (print "Print")))
    (funcall command-input :add-shortcut '((:f))
             (lambda () (push-interaction (fps-interaction))))
    (make-interaction
     :tick nil
     :keyboard-event (lambda (ev) (funcall command-input :feed ev))
     :mouse-event nil)))

(defun fps-interaction ()
  (make-interaction
   :on-enter (lambda () (input :cursor :capture t))
   :on-exit (lambda () (input :cursor :capture nil))
   :tick
   (lambda (dt)
     (let ((dx (input :mouse :delta :x))
           (dy (input :mouse :delta :y))
           (w-down (input :keyboard :w :down))
           (a-down (input :keyboard :a :down))
           (s-down (input :keyboard :s :down))
           (d-down (input :keyboard :d :down))
           (space-down (input :keyboard :space :down))
           (ctrl-down (input :keyboard :ctrl :down)))
       (let ((move-speed (* *keyboard-sensitivity* dt))
             (look-speed (* *mouse-sensitivity* dt)))
         (%renderer-camera-rotate (* (- dx) look-speed) (* (- dy) look-speed))
         (%renderer-camera-move
          (+ (if w-down move-speed 0.0)
             (if s-down (- move-speed) 0.0))
          (+ (if a-down (- move-speed) 0.0)
             (if d-down move-speed 0.0))
          (+ (if space-down move-speed 0.0)
             (if ctrl-down (- move-speed) 0.0))))))
   :keyboard-event
   (lambda (ev)
     (when (and (keyboard-event-down ev)
                (= (keyboard-event-scancode ev) (key :escape)))
       (exit-interaction)))
   :mouse-event
   (lambda (ev)
     (when (and (eq (mouse-event-type ev) :button)
                (= (mouse-event-button ev) 1) ; MOUSE_BUTTON_LEFT
                (mouse-event-down ev))
       (exit-interaction)))))

(defun editor ()
  (when (%renderer-init)
    (setf *interaction-stack* nil)
    (push-interaction (make-fundamental-2d-interaction))
    (unwind-protect
	 (loop until (%input-should-close-p)
	       doing
		  (%input-poll)
		  (let ((interaction (current-interaction)))
		    (when interaction
		      (funcall interaction :tick (%time-dt))
		      (do-input-events (:keyboard ev)
			(funcall interaction :keyboard-event ev))
		      (do-input-events (:mouse ev)
			(funcall interaction :mouse-event ev))))
		  (%renderer-render-begin)
		  (when *current-scene*
		    (scene-render *current-scene*))
		  (%ui-begin)
		  (panel-inspector)
		  (panel-instance-properties)
		  (%ui-end)
		  (%renderer-render-end))
      (let ((it (current-interaction)))
	(when it (funcall it :exit)))
      (setf *interaction-stack* nil
	    *current-scene* nil
	    *selection* nil)
      (do-scenes(scene)
	(scene-destroy scene))
      (%renderer-quit))))

;; (editor)
