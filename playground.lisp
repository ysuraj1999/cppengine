(in-package :playground)

(deftype u64 () '(unsigned-byte 64))

(defvar *selection* nil
  "Currently selected instance handle (a raw InstanceHandle).")

(defun selection-get (object)
  (eql *selection* object))

(defun selection-set (object)
  (setf *selection* object)
  object)

(defstruct prefix-node
  (table (make-hash-table :test #'equal))
  function)

(defun make-prefix-machine()
  (let* ((prefix-root (make-prefix-node))
	 (prefix-cursor prefix-root))
    (labels ((shortcut(keychords)
	       "Convert ((:ctrl :f) (:ctrl :x)) => ((9 224) (27 224))"

	       (mapcar #'(lambda(chord)
			   (sort (mapcar #'key-code chord) #'<))
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
		   (if (equal chord (list (key-code :escape)))
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

(defparameter *modifier-keys* (mapcar #'key-code '(:left-ctrl :left-shift :left-alt :left-gui
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
	  (:feed (destructuring-bind (input-event) args
		   (mu:with-members (type key down) input-event input-event
		     (case type
		       (:key-down (add-key (key-code key)))
		       (:key-up (remove-key (key-code key)))
		       (otherwise nil)))))
	  (:reset (setf held-keys nil
			held-modifiers nil
			chord-keys nil 
			has-key nil)))))))

(defun make-command-input()
  "Single dispatch lambda — mirrors chord/prefix machines.
     Interfaces: :feed (input-event), :reset, :add-shortcut (expr fn),
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
    (with-ui-window "Instance Properties"
      (let ((tr (instance-transform *selection*)))
        (mu:with-members (px py pz rx ry rz sx sy sz) tr transform
	  (c-ui-text (format nil "Position: ~,2f ~,2f ~,2f" px py pz))
	  (c-ui-text (format nil "Rotation: ~,2f ~,2f ~,2f" rx ry rz))
	  (c-ui-text (format nil "Scale: ~,2f ~,2f ~,2f" sx sy sz)))))))

(defun ui-mesh (mesh)
  "Mesh tree node. Double click adds a new instance of MESH."
  (with-ui-tree (mesh-alias mesh) mesh-open
    (ui-on-double-click :left
      (instance mesh))
    (ui-on-context
      (ui-menu-item "Instantiate"
        (instance mesh))
      (ui-menu-item "Delete Mesh"
        (when (remove-mesh mesh)
          (c-renderer-rebuild))))
    (when mesh-open
      (do-instances (instance mesh)
        (ui-instance instance)))))

(defun look-at-instance (instance)
  "Aim the camera at INSTANCE's position."
  (mu:with-members (px py pz) (instance-transform instance) transform
    (c-renderer-camera-look-at px py pz)))

(defun ui-instance (instance)
  "Instance selectable. Left click selects and points the camera at the
instance, middle click deletes."
  (with-ui-id instance
    (ui-selectable (instance-alias instance)
                   (selection-get instance))
    (ui-on-click :left
      (selection-set instance)
      (look-at-instance instance))
    (ui-on-click :middle
      (remove-instance instance))
    (ui-on-context
      (ui-menu-item "Delete"
        (remove-instance instance)))))

(defun panel-inspector ()
  (with-ui-window "Inspector"
    (ui-on-context-window
      (ui-menu-item "Load OBJ"
        (let ((path (file-picker)))
          (when path
            (load-obj path)
            (c-renderer-rebuild)))))
    (do-meshes (mesh)
      (ui-mesh mesh))))


(defparameter *keyboard-sensitivity* 200.0)
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

(defun make-interaction (&key tick event on-enter on-exit)
  "Build an interaction dispatch lambda (action &rest args) handling
   :tick (dt), :event (input-event), :enter, :exit. Unprovided
   callbacks default to no-ops."
  (let ((tick-fn   (or tick (lambda (dt) (declare (ignore dt)))))
        (event-fn  (or event (lambda (event) (declare (ignore event)))))
        (enter-fn  (or on-enter (lambda ())))
        (exit-fn   (or on-exit (lambda ()))))
    (lambda (action &rest args)
      (ecase action
        (:tick  (apply tick-fn args))
        (:event (apply event-fn args))
        (:enter (funcall enter-fn))
        (:exit  (funcall exit-fn))))))

(defun make-fundamental-2d-interaction ()
  (let* ((command-input (make-command-input)))
    (funcall command-input :add-shortcut '((:ctrl :s)) (lambda () (print "Save")))
    (funcall command-input :add-shortcut '((:ctrl :k)) (lambda () (print "Kill")))
    (funcall command-input :add-shortcut '((:ctrl :p)) (lambda () (print "Print")))
      (funcall command-input :add-shortcut '((:f))
               (lambda () (push-interaction (fps-interaction))))
      (funcall command-input :add-shortcut '((:t))
               (lambda ()
                 (when *selection*
                   (push-interaction (translate-interaction)))))
        (make-interaction
       :tick nil
       :event (lambda (ev) (funcall command-input :feed ev)))))

(defun increase-keyboard-sensitivity()
  (incf *keyboard-sensitivity* 10))

(defun decrease-keyboard-sensitivity()
  (decf *keyboard-sensitivity* 10))

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
       (let* ((speed (sqrt (+ (* dx dx) (* dy dy))))
	      (normalized-speed (clamp (/ speed 20.0) 0.0 1.0))
	      (scale (cubic-bezier
		      0.82 0.00 0.99 0.34
		      normalized-speed))
	      (move-speed (* *keyboard-sensitivity* dt))
              (look-speed (* *mouse-sensitivity* dt scale)))
         (c-renderer-camera-rotate (* (- dx) look-speed) (* (- dy) look-speed))
         (c-renderer-camera-move
          (+ (if w-down move-speed 0.0)
             (if s-down (- move-speed) 0.0))
          (+ (if a-down (- move-speed) 0.0)
             (if d-down move-speed 0.0))
          (+ (if space-down move-speed 0.0)
             (if ctrl-down (- move-speed) 0.0))))))
    :event
    (lambda (ev)
      (mu:with-members (type key button) ev input-event
	(when (and (eq type :key-down)
		   (eq key :minus))
	  (decrease-keyboard-sensitivity))
	(when (and (eq type :key-down)
		   (eq key :equals))
	  (increase-keyboard-sensitivity))
        (when (or (and (eq type :key-down)
                         (eq key :escape))
                  (and (eq type :mouse-button-down)
                         (eq button :left)))
            (exit-interaction))))))

(defun translate-interaction ()
  "Modal 3-axis translation of *SELECTION*: mouse dx -> z, dy -> y,
  scroll dz -> x, applied in world space one frame at a time via
  edit-transform. Exits on ESC."
  (make-interaction
   :on-enter (lambda () (input :cursor :capture t))
   :on-exit (lambda () (input :cursor :capture nil))
   :tick
   (lambda (dt)
     (when *selection*
       (let ((dx (input :mouse :delta :x))
             (dy (input :mouse :delta :y))
             (dz (input :mouse :wheel :y))
             (mouse-scale (* *mouse-sensitivity* dt)))
           (let ((tr (instance-transform *selection*)))
             (setf tr (edit-transform (tr)
			(:move-by :z (* dx mouse-scale))
			(:move-by :y (* dy mouse-scale))
			(:move-by :x (* dz mouse-scale))))
             (setf (instance-transform *selection*) tr)))))
   :event
   (lambda (ev)
     (mu:with-members (type key button) ev input-event
       (when (or (and (eq type :mouse-button-down)
		      (eq button :left))
		 (and (eq type :key-down)
                      (eq key :escape)))
	 (exit-interaction))))))

(defun cubic-bezier (x1 y1 x2 y2 input)
  (labels ((bezier-value (p0 p1 p2 p3 parameter)
             (let ((u (- 1.0 parameter)))
               (+ (* u u u p0)
                  (* 3.0 u u parameter p1)
                  (* 3.0 u parameter parameter p2)
                  (* parameter parameter parameter p3))))
           
           (bezier-x (parameter)
             (bezier-value 0.0 x1 x2 1.0 parameter))
           
           (bezier-y (parameter)
             (bezier-value 0.0 y1 y2 1.0 parameter)))
    
    (let ((parameter 0.5))
      (dotimes (i 20)
        (let ((current-x (bezier-x parameter)))
          (setf parameter
                (clamp (+ parameter
                          (* (- input current-x) 0.5))
                       0.0
                       1.0))))
      (bezier-y parameter))))

(defun editor ()
  (when (c-renderer-init)
    (setf *interaction-stack* nil)
    (push-interaction (make-fundamental-2d-interaction))
    (unwind-protect
	 (loop until (c-input-should-close-p)
	       doing
		  (c-input-poll)
		  (let ((interaction (current-interaction)))
		    (when interaction
		      (funcall interaction :tick (c-time-dt))
		      (do-input-events (ev)
			(funcall interaction :event ev))))
		  (c-renderer-render-begin)
		  (c-renderer-render-scene)
		  (c-ui-begin)
		  (panel-inspector)
		  (panel-instance-properties)
		  (c-ui-end)
		  (c-renderer-render-end))
      (let ((it (current-interaction)))
	(when it (funcall it :exit)))
      (setf *interaction-stack* nil
	    *selection* nil)
      (c-renderer-quit))))

;; (editor)

(defun snake()
  (when (c-renderer-init)
    (let ((grid-size 15)
	  (direction :right)
	  (running nil)
	  (last-tail-position)
	  (cube)
	  (food)
	  (snake (make-array 0 :adjustable t :fill-pointer 0))
	  (accumulator 0)
	  (step-rate (coerce (/ 1.0 5.0) 'single-float))) ; 5 ticks per second
      (labels ((init()
		 (load-obj "./models/cube.obj")
		 (c-renderer-rebuild)
		 (vector-push-extend '(0.0 0.0) snake)
		 (do-meshes (mesh)
		   (setf cube mesh))
		 (place-food))
	       (place-food()
		 (setf food (list (coerce (- (random grid-size) 7) 'single-float) (coerce (- (random grid-size) 7) 'single-float))))
	       (toggle()
		 (setf running (not running)))
	       (move-ahead()
		 (let* ((head (aref snake 0))
			(new-head
			  (destructuring-bind (x y) head
			    (ecase direction
			      (:up (list x (1+ y)))
			      (:down (list x (1- y)))
			      (:right (list (1- x) y))
			      (:left (list (1+ x) y))))))
		   (setf last-tail-position (aref snake (1- (length snake))))
		   (loop for current-index downfrom (1- (length snake)) to 1
			 for previous = (aref snake (1- current-index))
			 doing
			    (setf (aref snake current-index) previous))
		   (setf (aref snake 0) new-head)
		   (format t "Head ~a, Food ~a~%" new-head food)
		   (if (equal new-head food)
		       (grow))))
	       (grow()
		 (vector-push-extend last-tail-position snake)
		 (place-food))
	       (snake-to-cubes()
		 (ensure-instance-count cube (1+ (length snake)))
		 (dotimes (c (length snake))
		   (destructuring-bind (x y) (aref snake c)
		     (let* ((i (c-mesh-instance-at cube c))
			    (tr (c-instance-get-transform i)))
		       (c-instance-set-transform i
						 (edit-transform (tr)
						   (:position :z x)
						   (:position :y y))))))
		 (let* ((i (c-mesh-instance-at cube (length snake)))
			(food-tr (transform! 0.0 0.0 0.0
					    1.0 1.0 1.0
					    0.0 0.0 0.0)))
		   (c-instance-set-transform i
					     (edit-transform (food-tr)
							     (:position :z (first food))
							     (:position :y (second food)))))))

	(init)
	(snake-to-cubes)
	(let ((snake-interaction
		(make-interaction
		 :tick (lambda(dt)
			 (incf accumulator dt)
			 (when (>= accumulator step-rate)
			   (decf accumulator step-rate)
			   ;; (loop for segment across snake
			   ;; 	 doing (format t "~a~%" segment))
			   (and running
				(progn (move-ahead)
				       (snake-to-cubes)))))
		 :event (lambda (ev)
			  (mu:with-members (type key button) ev input-event
			    (cond
			      ((and (eq type :key-down) (eq key :w))
			       (setf direction :up))
			      ((and (eq type :key-down) (eq key :a))
			       (setf direction :left))
			      ((and (eq type :key-down) (eq key :s))
			       (setf direction :down))
			      ((and (eq type :key-down) (eq key :d))
			       (setf direction :right))
			      ((and (eq type :key-down) (eq key :space))
			       (toggle))
			      ((and (eq type :key-down) (eq key :g))
			       (grow))
			      ((and (eq type :key-down) (eq key :f))
			       (push-interaction (fps-interaction)))))))))
	  (push-interaction snake-interaction)
	  (loop until (c-input-should-close-p)
		doing
		   (c-input-poll)
		   (let ((interaction (current-interaction)))
		     (when interaction
		       (funcall interaction :tick (c-time-dt))
		       (do-input-events (ev)
			 (funcall interaction :event ev))))
		   (c-renderer-render-begin)
		   (c-renderer-render-scene)
		   (c-ui-begin)
		   (c-ui-end)
		   (c-renderer-render-end))
	  (c-renderer-quit))))))
