(ns baliset-ui.views
  (:require [re-frame.core :as rf]
            [cljsjs.hammer]
            [goog.object :as goo]
            [reagent.core :as re :refer [create-class]]))

(defn- translate [x y]
  (str "translate(" x "px, " y "px)"))

(defn- scale [x y]
  (str "scale(" x ", " y ")"))

(defn connection
  "A bezier curve representing a connection between an inlet and an outlet"
  [c]
  (let [[out-x out-y cx1 cy1 cx2 cy2 in-x in-y] @(rf/subscribe [:connection c])]
    [:path {:d (str "M" in-x " " in-y " "
                    "C" cx1 " " cy1 " "
                    cx2 " " cy2 " "
                    out-x " " out-y)}]))

(defn connections
  "An svg element containing lines that represent connections between inlets and outlets."
  []
  [:svg.noclick
   [:g
    (for [c @(rf/subscribe [:connections])]
      ^{:key (str "connection:" c)}
      [connection c])]])

(defn inlet
  [node-id idx name]
  (let [selected? @(rf/subscribe [:selected-io? :inlet node-id idx])]
    [(if selected? :div.inlet.selected-inlet :div.inlet)
     {:ref (fn [this]
             (if this
               (rf/dispatch [:reg-inlet-offset node-id idx (goo/get this "offsetTop") (goo/get this "offsetHeight")])
               (rf/dispatch [:unreg-inlet-offset node-id idx])))
      :on-click (fn [e]
                  (.stopPropagation e)
                  (rf/dispatch [:clicked-inlet node-id idx]))}
     name]))

(defn outlet [node-id idx name]
  (let [selected? @(rf/subscribe [:selected-io? :outlet node-id idx])]
    [(if selected? :div.outlet.selected-outlet :div.outlet)
     {:on-click (fn [e]
                  (.stopPropagation e)
                  (rf/dispatch [:clicked-outlet node-id idx]))
      :ref (fn [this]
             (if this
               (rf/dispatch [:reg-outlet-offset node-id idx
                             (goo/get this "offsetTop") (goo/get this "offsetHeight")
                             (goo/get this "offsetLeft") (goo/get this "offsetWidth")])
               (rf/dispatch [:unreg-outlet-offset node-id idx])))}
     name]))

(defonce touch-lock (atom nil))

(defn node
  "A div representing a node. Has a fixed left/top position, but can be dragged around the patch.
  During the drag events, the position changes via transform: translate. One the drag has stopped,
  left/top is reset to the new position and the new position is shared with other websocket clients."
  [id]
  (let [touch-state (atom {})]
    (create-class
     {:display-name (str "node-" id)
      :component-did-mount
      (fn [this]
        (let [dom-node (re/dom-node this)
              ham-man (new js/Hammer.Manager dom-node)]
          (.add ham-man (new js/Hammer.Pan #js {"event" "pan"}))
          (.on ham-man "pan panstart panend pancancel"
               (fn [ev]
                 (when (or (nil? @touch-lock) (= @touch-lock :node))
                   (cond (= "panstart" (goo/get ev "type"))
                         (do (swap! touch-state assoc :moving true)
                             (reset! touch-lock :node)
                             (rf/dispatch [:drag-node id (goo/get ev "deltaX") (goo/get ev "deltaY")]))

                         (and (:moving @touch-state) (goo/get ev "isFinal"))
                         (do (swap! touch-state assoc :moving false)
                             (reset! touch-lock nil)
                             (rf/dispatch [:finish-drag-node id]))

                         (:moving @touch-state)
                         (rf/dispatch [:drag-node id (goo/get ev "deltaX") (goo/get ev "deltaY")])))))
          (swap! touch-state assoc :ham-man ham-man)))

      :component-will-unmount
      (fn [this] (when-let [ham-man (:ham-man @touch-state)] (.destroy ham-man)))

      :reagent-render
      (fn [id]
        (let [node-data @(rf/subscribe [:node id])
              node-meta @(rf/subscribe [:node-metadata (:type node-data)])
              recently-interacted? @(rf/subscribe [:recently-interacted-node? id])
              [x y] @(rf/subscribe [:node-position id])]
          [:div.node {:style {:position "fixed"
                              :transform (translate x y)
                              :z-index (if recently-interacted? 1 0)}}
           [:div.node-header
            {:on-click (fn [e]
                         (.stopPropagation e)
                         (rf/dispatch [:clicked-node-header id]))}
            (get node-meta "name")]
           [:div.node-io
            (when-let [inlets (get node-meta "inlets")]
              [:div.inlets
               (map-indexed
                (fn [idx name]
                  ^{:key (str "inlet-" id "-" name)}
                  [inlet id idx name])
                inlets)])
            (when-let [outlets (get node-meta "outlets")]
              [:div.outlets
               (map-indexed
                (fn [idx name]
                  ^{:key (str "outlet-" id "-" name)}
                  [outlet id idx name])
                outlets)])]]))})))

(defn nodes
  "Container div for the nodes"
  []
  (let [node-ids @(rf/subscribe [:node-ids])]
    [:div
     (map (fn [id]
            ^{:key (str "node:" id)}
            [node id])
          node-ids)]))

(defn add-btn [node-name]
  [(if @(rf/subscribe [:selected-add-btn? node-name])
     :div.add-btn.selected-add-btn
     :div.add-btn)
   {:on-click
    (fn [e]
      (.stopPropagation e)
      (rf/dispatch [:clicked-add-btn node-name]))}
   (str node-name)])

(defn minimized-app-panel []
  [:div.minimized-app-panel
   {:on-click #(do (println "clicked minimized app panel")(rf/dispatch [:clicked-minimized-app-panel]))}
   [:svg.noclick [:g
                  [:line {:x1 0 :y1 4 :x2 24 :y2 4}]
                  [:line {:x1 0 :y1 10 :x2 24 :y2 10}]
                  [:line {:x1 0 :y1 16 :x2 24 :y2 16}]]]])

(defn nodes-btn []
  (let [selected-detail @(rf/subscribe [:selected-detail])]
    [(if (= :nodes selected-detail) :div.menu-btn.selected  :div.menu-btn)
     {:on-click (fn [e] (.stopPropagation e)
                  (rf/dispatch [:clicked-nodes-btn]))}
     "nodes"]))

(defn patches-btn []
  (let [selected-detail @(rf/subscribe [:selected-detail])]
    [(if (= :patches selected-detail) :div.menu-btn.selected :div.menu-btn)
     {:on-click (fn [e] (.stopPropagation e)
                  (rf/dispatch [:clicked-patches-btn]))}
     "patches"]))

(defn project-panel []
  [:div.app-panel-column.project
   [patches-btn]
   [nodes-btn]])

(defn nodes-detail []
  [:div
   (let [node-types @(rf/subscribe [:node-types])]
     (if node-types
       (for [t node-types]
         ^{:key (str "add-btn-" t)}
         [add-btn t])
       [:div]))])

(defn patches-list []
  (let [patches @(rf/subscribe [:patches])]
    [:div
     (doall (for [patch-name patches]
              ^{:key (str "p." patch-name)}
              [:div.menu-btn
               {:on-click (fn [e] (.stopPropagation e)
                            (rf/dispatch [:load-patch patch-name]))}
               (.slice patch-name 0 -5)]))]))

(defn patches-detail []
  [:div
   [:h1 "save:"]
   [:input.patch-name-input {:type "text"
                             :on-change #(rf/dispatch
                                          [:patch-save-name (-> % .-target .-value)])
                             :on-click (fn [e] (.stopPropagation e))}]
   [:div.menu-btn {:on-click (fn [e]
                               (.stopPropagation e)
                               (rf/dispatch [:save-patch]))}
    "save"]
   [:h1 "load:"]
   [patches-list]])

(defn detail-panel []
  [:div.app-panel-column.detail
   (let [selected-detail @(rf/subscribe [:selected-detail])]
     (case selected-detail
       :nodes
       [nodes-detail]

       :patches
       [patches-detail]
       [:div]))])

(defn expanded-app-panel []
  [:div.app-panel
   {:on-click #(rf/dispatch [:clicked-expanded-app-panel])}
   [project-panel]
   [detail-panel]])

(defn app-panel []
  (if @(rf/subscribe [:app-panel-expanded?])
    [expanded-app-panel]
    [minimized-app-panel]))

(defn delete-node-btn []
  [:div.delete-btn
   {:on-click (fn [e]
                (.stopPropagation e)
                (rf/dispatch [:clicked-delete-node-btn]))}
   "DELETE"])

(defn hslider [node-info ctl-id ctl]
  (let [x-off 4
        y-off 6
        width 324
        height 8
        node-type (:type node-info)
        node-id (:id node-info)]
    (create-class
     {:display-name (str "hslider-" (:id node-info) (get ctl "name"))
      :component-did-mount
      (fn [this]
        (let [dom-node (re/dom-node this)
              ham-man (new js/Hammer.Manager dom-node)]
          (.add ham-man (new js/Hammer.Pan #js {"event" "pan" "threshold" 0}))
          (.on ham-man "pan panstart panend pancancel"
               (fn [ev]
                 (if (goo/get ev "isFinal")
                   (rf/dispatch [:finish-dragging-hslider node-type node-id ctl-id])
                   (rf/dispatch [:drag-hslider node-type node-id ctl-id (goo/get ev "deltaX") width]))))))
      :reagent-render
      (fn [node-info ctl-id ctl]
        (let [ctl-metadata @(rf/subscribe [:ctl-meta node-type ctl-id])
              [min max] (or (get ctl-metadata "range") [0.0 1.0])
              value (or @(rf/subscribe [:hslider-value node-type node-id ctl-id]) 0.0)
              percentage (/ (- value min) (- max min))]
          [:svg.hslider
           [:g [:text {:x 8 :y 0 :fill "#fff"} (.toFixed value 2)]
            [:rect {:x x-off :y y-off :width width :height height :rx 4 :ry 4 :fill "#333"}]
            [:rect {:x x-off :y y-off :width (* percentage width) :height height :rx 4 :ry 4 :fill "#2bb"}]
            [:circle {:cx (+ x-off (* percentage width)) :cy (+ y-off (/ height 2)) :r 7 :fill "#2bb"}]]]))})))

(defn trigger [node-info ctl-id ctl]
  (let [triggered? (re/atom nil)]
    (fn [node-info ctl-id ctl]
      [:svg.trigger [:g {:on-mouse-down (fn [e]
                                          (reset! triggered? true)
                                          (rf/dispatch [:trigger (:id node-info) ctl-id 1.0]))
                         :on-mouse-up (fn [e]
                                        (reset! triggered? false)
                                        (rf/dispatch [:trigger (:id node-info) ctl-id 0.0]))}
                     [:rect.raised {:x 4 :y 6 :width 32 :height 32 :fill "#333"}]
                     (when @triggered? [:rect.raised {:x 7 :y 9 :width 26 :height 26 :fill "#2bb"}])]])))

(defn control [node-info ctl-id ctl]
  [:div.ctl
   [:p (get ctl "name")]
   (case (get ctl "type")
     "hslider"
     [hslider node-info ctl-id ctl]

     "trigger"
     [trigger node-info ctl-id ctl])])

(defn controls [node-info]
  (let [node-meta @(rf/subscribe [:node-metadata (:type node-info)])
        controls (get node-meta "controls")]
    [:div.controls
     (map-indexed
      (fn [ctl-id ctl]
        ^{:key (str "ctl:" (:id node-info) (get ctl "name"))}
        [control node-info ctl-id ctl])
      controls)]))

(defn node-panel []
  (let [expanded? @(rf/subscribe [:node-panel-expanded?])
        node-info @(rf/subscribe [:selected-node])]
    (cond (and node-info expanded?)
          [:div.node-panel
           {:on-click (fn [e] (.stopPropagation e))}
           [:h1 (str (:type node-info) " " (:id node-info))]
           [controls node-info]
           [delete-node-btn]]

          :default
          [:div])))

(defn canvas
  "A zero-sized div that shows its contents via overflow: visible.
  Using transform: translate on this div allows us to 'move' the patch"
  []
  (let [[x y] @(rf/subscribe [:pan-pos])
        scl @(rf/subscribe [:scale])]
    [:div.canvas
     {:style {:transform (str (scale scl scl) (translate x y))}}
     [nodes]
     [connections]]))

(defn app
  "Top-level div, containing all other parts of the app. Sized to the screen dimensions"
  []
  (let [touch-state (atom {})]
    (create-class
     {:display-name "app"
      :component-did-mount
      (fn [this]
        (let [dom-node (re/dom-node this)
              ham-man (new js/Hammer.Manager dom-node)]
          (.add ham-man (new js/Hammer.Pan #js {"event" "pan"}))
          (.add ham-man (new js/Hammer.Pinch #js {"event" "pinch"}))
          (.on ham-man "pan panstart panend"
               (fn [ev]
                 (when (or (nil? @touch-lock) (= @touch-lock :pan-app))
                   (cond (and (= "panstart" (goo/get ev "type")) (= (goo/get ev "target") dom-node))
                         (do (swap! touch-state assoc :moving true)
                             (reset! touch-lock :pan-app)
                             (rf/dispatch [:pan-camera (goo/get ev "deltaX") (goo/get ev "deltaY")]))

                         (and (:moving @touch-state) (goo/get ev "isFinal"))
                         (do (swap! touch-state assoc :moving false)
                             (reset! touch-lock nil)
                             (rf/dispatch [:finish-pan-camera]))

                         (:moving @touch-state)
                         (rf/dispatch [:pan-camera (goo/get ev "deltaX") (goo/get ev "deltaY")]))))
               (swap! touch-state assoc :ham-man ham-man))
          (.on ham-man "pinch pinchstart pinchend pinchcancel"
               (fn [ev]
                 (let [lock @touch-lock]
                   (cond (and (nil? lock) (= (goo/get ev "type") "pinchstart"))
                         (do (reset! touch-lock :pinch)
                             (rf/dispatch [:pinched-app (goo/get ev "scale") (goo/get ev "center")]))

                         (and (= :pinch lock)
                              (or (= (goo/get ev "type") "pinchcancel") (= (goo/get ev "type") "pinchend")))
                         (do (reset! touch-lock nil)
                             (rf/dispatch [:stopped-pinching-app]))

                         (= :pinch lock)
                         (rf/dispatch [:pinched-app (goo/get ev "scale") (goo/get ev "center")])))))))

      :component-will-unmount
      (fn [this] (when-let [ham-man (:ham-man @touch-state)] (.destroy ham-man)))

      :reagent-render
      (fn []
        [:div.app {:on-click #(rf/dispatch [:clicked-app-div
                                            (goo/getValueByKeys % "nativeEvent" "clientX")
                                            (goo/getValueByKeys % "nativeEvent" "clientY")])
                   :on-wheel (fn [e]
                               (rf/dispatch [:scrolled-wheel (goo/get e "deltaY") (goo/get e "clientX") (goo/get e "clientY")]))}
         [app-panel]
         [node-panel]
         [canvas]])})))


;; (comment
;;   (+ 3 3)
;;   (def sock (js/WebSocket. "ws://localhost:3002"))
;;   (.send sock (rand-int 4000))

;;   (do
;;     (.send sock (.stringify js/JSON #js {:route "/node/add" :type "sin"}))
;;     (.send sock (.stringify js/JSON #js {:route "/node/add" :type "dac"}))

;;     (.send sock (.stringify js/JSON #js {:route "/node/connect"
;;                                          :out_node_id 0
;;                                          :outlet_id 0
;;                                          :in_node_id 2
;;                                          :inlet_id 0})))

;;   (.send sock (.stringify js/JSON #js {:route "/node/disconnect"
;;                                        :out_node_id 0
;;                                        :outlet_id 0
;;                                        :in_node_id 2
;;                                        :inlet_id 0}))

;;   (.send sock (.stringify js/JSON #js {:route "/node/add" :type "sin"}))
;;   (.send sock (.stringify js/JSON #js {:route "/node/delete"
;;                                        :node_id 0}))

;;   (.addEventListener sock
;;                      "open"
;;                      (fn [e] (.send sock "Hullo, Server!")))
;;   (str (.-location js/window))
;;   js/Hammer
;; )
