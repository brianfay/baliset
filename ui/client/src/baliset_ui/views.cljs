(ns baliset-ui.views
  (:require [re-frame.core :as rf]
            [cljsjs.hammer]
            [reagent.core :as re :refer [create-class]]))

(defn- translate [x y]
  (str "translate(" x "px, " y "px"))

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
  [:svg [:g
         (for [c @(rf/subscribe [:connections])]
           ^{:key (str "connection:" c)}
           [connection c])]])

(defn inlet
  [node-id idx name]
  (let [selected? @(rf/subscribe [:selected? :inlet node-id idx])]
    [(if selected? :div.inlet.selected-inlet :div.inlet)
     {:ref (fn [this]
             (if this
               (rf/dispatch [:reg-inlet-offset node-id idx (.-offsetTop this) (.-offsetHeight this)])
               (rf/dispatch [:unreg-inlet-offset node-id idx])))
      :on-click (fn [e] (rf/dispatch [:clicked-inlet node-id idx]))}
     name]))

(defn outlet [node-id idx name]
  (let [selected? @(rf/subscribe [:selected? :outlet node-id idx])]
    [(if selected? :div.outlet.selected-outlet :div.outlet)
     {:on-click (fn [e] (rf/dispatch [:clicked-outlet node-id idx]))
      :ref (fn [this]
             (if this
               (rf/dispatch [:reg-outlet-offset node-id idx
                             (.-offsetTop this) (.-offsetHeight this)
                             (.-offsetLeft this) (.-offsetWidth this)])
               (rf/dispatch [:unreg-outlet-offset node-id idx])))}
     name]))

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
                 (.stopPropagation (.-srcEvent ev))
                 (cond (and (= "panstart" (.-type ev)))
                       (do (swap! touch-state assoc :moving true)
                           (rf/dispatch [:drag-node id (.-deltaX ev) (.-deltaY ev)]))

                       (or (= "panend" (.-type ev)) (= "pancancel" (.-type ev)))
                       (do (swap! touch-state assoc :moving false)
                           (rf/dispatch [:finish-drag-node id]))

                       (:moving @touch-state)
                       (rf/dispatch [:drag-node id (.-deltaX ev) (.-deltaY ev)]))))
          (swap! touch-state assoc :ham-man ham-man)))

      :component-will-unmount
      (fn [this] (when-let [ham-man (:ham-man @touch-state)] (.destroy ham-man)))

      :reagent-render
      (fn [id]
        (let [node-data @(rf/subscribe [:node id])
              node-meta @(rf/subscribe [:node-metadata (:type node-data)])
              [x y] @(rf/subscribe [:node-position id])]
          [:div.node {:style {:position "fixed"
                              :transform (translate x y)}}
           [:div.node-header (get node-meta "name")]
           [:div.node-io
            [:div.inlets
             (map-indexed
              (fn [idx name]
                ^{:key (str "inlet-" id "-" name)}
                [inlet id idx name])
              (get node-meta "inlets"))]
            [:div.outlets
             (map-indexed
              (fn [idx name]
                ^{:key (str "outlet-" id "-" name)}
                [outlet id idx name])
              (get node-meta "outlets"))]]]))})))

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
  [:div.add-btn
   {:on-click
    (fn [e]
      (.stopPropagation e)
      (rf/dispatch [:add-node node-name]))}
   (str node-name)])

(defn left-nav []
  (let [node-types @(rf/subscribe [:node-types])]
    [:div.left-nav
     [:h1 "baliset"]
     [:h2 "add-node"]
     (if node-types
       (for [t node-types]
         ^{:key (str "add-btn-" t)}
         [add-btn t])
       [:div])]))

(defn canvas
  "A zero-sized div that shows its contents via overflow: visible.
  Using transform: translate on this div allows us to 'move' the patch"
  []
  (let [[x y] @(rf/subscribe [:pan-pos])]
    [:div.canvas
     {:style {:transform (translate x y)}}
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
          (.on ham-man "pan panstart panend"
               (fn [ev]
                 (cond (and (= "panstart" (.-type ev)) (= (.-target ev) dom-node))
                       (do (swap! touch-state assoc :moving true)
                           (rf/dispatch [:pan-camera (.-deltaX ev) (.-deltaY ev)]))

                       ;;TODO this panning handles poorly on desktop browser, moves both node and canvas at same time
                       (.-isFinal ev)
                       (do (swap! touch-state assoc :moving false)
                           (rf/dispatch [:finish-pan-camera]))

                       (:moving @touch-state)
                       (rf/dispatch [:pan-camera (.-deltaX ev) (.-deltaY ev)])))
               (swap! touch-state assoc :ham-man ham-man))))

      :component-will-unmount
      (fn [this] (when-let [ham-man (:ham-man @touch-state)] (.destroy ham-man)))

      :reagent-render
      (fn []
        [:div.app
         [left-nav]
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
