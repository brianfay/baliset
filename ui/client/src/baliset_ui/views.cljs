(ns baliset-ui.views
  (:require [re-frame.core :as rf]))

(defn header [name]
  [:div.node-header name])

(defn inlet [name]
  [:div.inlet name])

(defn outlet [name]
  [:div.outlet name])

(defn io []
  [:div.node-io
   [:div.inlets
    [inlet "in"]
    [inlet "trig"]]
   [:div.outlets [outlet "out"]]]
  )

(defn node
  []
  [:div.node {:style {:position "fixed"
                      :left 0
                      :top 0}}
   [header "looper"]
   [io]])

(defn add-btn [node-name]
  [:div.add-btn
   {:on-click
    (fn [e]
      (.stopPropagation e)
      (rf/dispatch [:add-node node-name]))}
   (str node-name)])

(defn left-nav []
  (let [node-metadata @(rf/subscribe [:node-metadata])]
    [:div.left-nav
     (if node-metadata
       (for [node-name (map str (keys node-metadata))]
         ^{:key (str "add-btn-" node-name)}
         [add-btn node-name])
       [:div])
     ]))

  (defn app []
    [:div
     [left-nav]
     [node]
     ]
    )

