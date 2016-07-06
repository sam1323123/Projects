//
//  buttonFactory.swift
//  Midterm Project- Finance app
//
//  Created by Samuel Lee on 2/15/15.
//  Copyright (c) 2015 Samuel Lee. All rights reserved.
//

import UIKit

enum buttonType {
    case number
    case op
    case equal
}

class buttonFactory: NSObject {
    class func buttonWithType(type: buttonType, label: String) -> UIButton {
        let button = UIButton.buttonWithType(UIButtonType.Custom) as UIButton
        button.setTitle(label, forState: UIControlState.Normal)
        button.titleLabel!.font = UIFont(name: "Verdana-Bold", size: 15.0)
        button.backgroundColor = UIColor.blackColor()
        button.setTitleColor(UIColor.blackColor(), forState: UIControlState.Normal)
        button.showsTouchWhenHighlighted = true
        var width = 0
        var height: Int
        height = width
        var buttonSize = CGRect(origin: CGPointZero, size: CGSize(width: width, height: height))
        button.frame = buttonSize
        var layer = button.layer
        layer.borderColor = UIColor.blackColor().CGColor
        layer.borderWidth = 1.2
        layer.cornerRadius = 6.0
        switch type {
        case buttonType.number:
            button.setBackgroundImage(UIImage(named: "red"), forState: UIControlState.Normal)
            break
        case buttonType.op:
            button.backgroundColor = UIColor(red: 30.0/255, green: 60.0/255, blue: 180.0/255, alpha: 1.0)
            button.setBackgroundImage(UIImage(named: "darkblue"), forState: UIControlState.Normal)
        case buttonType.equal:
            button.backgroundColor = UIColor.blackColor()
            button.setBackgroundImage(UIImage(named: "darkblue"), forState: UIControlState.Normal)
            var buttonSize = CGRect(origin: CGPointZero, size: CGSize(width: width * 2, height: height))
            button.frame = buttonSize

        default:
            break
            
        }
        return button
    }
   
}
