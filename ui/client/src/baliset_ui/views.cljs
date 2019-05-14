(ns baliset-ui.views
  (:require [re-frame.core :as rf]
            [cljsjs.hammer]
            [reagent.core :as re :refer [create-class]]))

(defn- translate3d [x y z]
  (str "translate3d(" x "px, " y "px, " z "px)"))

(defn inlet [name]
  [:div.inlet name])

(defn outlet [name]
  [:div.outlet name])

(defn node
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
              [off-x off-y] @(rf/subscribe [:node-offset id])]
          [:div.node {:style {:position "fixed"
                              :left (:x node-data)
                              :top (:y node-data)
                              :transform (translate3d off-x off-y 0)}}
           [:div.node-header (get node-meta "name")]
           [:div.node-io
            [:div.inlets
             (for [in (get node-meta "inlets")]
               ^{:key (str "inlet-" id "-" in)}
               [inlet in])]
            [:div.outlets
             (for [out (get node-meta "outlets")]
               ^{:key (str "outlet-" id out)}
               [outlet out])]]]))})))

(defn nodes
  []
  (let [node-ids @(rf/subscribe [:node-ids])]
    (def node-ids node-ids)
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

(defn canvas []
  (let [[x y] @(rf/subscribe [:pan])]
    [:div.canvas
     {:style {:transform (translate3d x y 0)}}
     [nodes]]))

(defn app []
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
